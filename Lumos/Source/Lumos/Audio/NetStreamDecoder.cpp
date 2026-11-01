#include "Precompiled.h"
#include "NetStreamDecoder.h"
#include "Core/OS/HttpStream.h"
#include "Maths/MathsUtilities.h"

#include <dr_libs/dr_mp3.h>
#include <chrono>
#include <cstring>

namespace Lumos
{
    namespace
    {
        size_t OnRead(void* user, void* out, size_t bytes)
        {
            return static_cast<NetStreamDecoder*>(user)->ReadCompressed((u8*)out, bytes);
        }
    }

    NetStreamDecoder::NetStreamDecoder(const std::string& url)
        : m_Url(url)
    {
        m_Codec = "mp3";
        m_Thread = std::thread([this]() { Run(); });
    }

    NetStreamDecoder::~NetStreamDecoder()
    {
        m_Stop = true;
        if(m_Thread.joinable())
            m_Thread.join();
        delete m_Http;
    }

    std::string NetStreamDecoder::GetError() const
    {
        std::lock_guard<std::mutex> lock(m_Lock);
        return m_Error;
    }

    std::string NetStreamDecoder::GetStationName() const
    {
        std::lock_guard<std::mutex> lock(m_Lock);
        return m_Station;
    }

    std::string NetStreamDecoder::GetStreamTitle() const
    {
        std::lock_guard<std::mutex> lock(m_Lock);
        return m_Title;
    }

    void NetStreamDecoder::Fail(const std::string& error)
    {
        {
            std::lock_guard<std::mutex> lock(m_Lock);
            m_Error = error;
        }
        m_State = State::Failed;
    }

    void NetStreamDecoder::ParseMetadata(const std::string& block)
    {
        // StreamTitle='Artist - Song';StreamUrl='...';
        size_t at = block.find("StreamTitle='");
        if(at == std::string::npos)
            return;
        at += 13;
        size_t end = block.find("';", at);
        if(end == std::string::npos)
            end = block.find('\'', at);
        std::lock_guard<std::mutex> lock(m_Lock);
        m_Title = block.substr(at, end == std::string::npos ? std::string::npos : end - at);
    }

    size_t NetStreamDecoder::ReadCompressed(u8* out, size_t bytes)
    {
        size_t got = 0;
        while(got < bytes && !m_Stop.load())
        {
            if(m_Http->IsFinished())
                break;
            size_t want = bytes - got;
            if(m_MetaInt > 0)
            {
                if(m_UntilMeta == 0)
                {
                    // one length byte (x16) then that much metadata text
                    u8 len = 0;
                    if(m_Http->Read(&len, 1, 500) == 0)
                        continue;
                    std::string meta(len * 16, '\0');
                    size_t have = 0;
                    while(have < meta.size() && !m_Stop.load() && !m_Http->IsFinished())
                        have += m_Http->Read((u8*)&meta[have], meta.size() - have, 500);
                    if(len > 0)
                        ParseMetadata(meta);
                    m_UntilMeta = m_MetaInt;
                    continue;
                }
                want = Maths::Min<size_t>(want, m_UntilMeta);
            }
            size_t n = m_Http->Read(out + got, want, 500);
            got += n;
            if(m_MetaInt > 0)
                m_UntilMeta -= (u32)n;
        }
        return got;
    }

    void NetStreamDecoder::Run()
    {
        m_Http = HttpStream::Open(m_Url, { { "Icy-MetaData", "1" }, { "User-Agent", "LumosTunes/0.2" } });
        while(!m_Http->HasResponse() && !m_Stop.load())
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        if(m_Stop.load())
            return;
        int status = m_Http->GetStatus();
        if(status != 200)
        {
            std::string err = m_Http->GetError();
            Fail(!err.empty() ? err : ("HTTP " + std::to_string(status)));
            return;
        }
        std::string type = m_Http->GetHeader("content-type");
        if(!type.empty() && type.find("mpeg") == std::string::npos && type.find("mp3") == std::string::npos)
        {
            Fail("not an MP3 stream (" + type + ")");
            return;
        }
        {
            std::lock_guard<std::mutex> lock(m_Lock);
            m_Station = m_Http->GetHeader("icy-name");
        }
        std::string metaint = m_Http->GetHeader("icy-metaint");
        m_MetaInt   = metaint.empty() ? 0 : (u32)atoi(metaint.c_str());
        m_UntilMeta = m_MetaInt;

        drmp3 mp3 = {};
        // no seek / tell: dr_mp3 then reads past tags and resyncs instead of seeking a live stream
        if(!drmp3_init(&mp3, OnRead, nullptr, nullptr, nullptr, this, nullptr))
        {
            Fail(m_Stop.load() ? "stopped" : "couldn't decode the stream");
            return;
        }
        m_SampleRate = mp3.sampleRate;
        m_Channels   = Maths::Clamp(mp3.channels, 1u, 2u);
        m_RingFrames = m_SampleRate * 8;
        {
            std::lock_guard<std::mutex> lock(m_Lock);
            m_Ring.Resize((size_t)m_RingFrames * m_Channels, 0);
        }

        const u32 prebuffer = m_SampleRate * 3 / 2;
        i16 chunk[1152 * 2];
        while(!m_Stop.load())
        {
            u64 used;
            {
                std::lock_guard<std::mutex> lock(m_Lock);
                used = (m_RingWrite - m_RingRead) / m_Channels;
            }
            if(used + 1152 > m_RingFrames)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(15));
                continue;
            }
            u32 n = (u32)drmp3_read_pcm_frames_s16(&mp3, 1152, chunk);
            if(n == 0)
            {
                if(m_Http->IsFinished() || m_Stop.load())
                {
                    m_State = State::Ended;
                    break;
                }
                continue;
            }
            {
                std::lock_guard<std::mutex> lock(m_Lock);
                const u64 cap = (u64)m_RingFrames * m_Channels;
                for(u32 i = 0; i < n * m_Channels; i++)
                    m_Ring[(size_t)((m_RingWrite + i) % cap)] = chunk[i];
                m_RingWrite += (u64)n * m_Channels;
                used = (m_RingWrite - m_RingRead) / m_Channels;
            }
            if(m_State.load() == State::Connecting && used >= prebuffer)
                m_State = State::Ready;
        }
        drmp3_uninit(&mp3);
    }

    u32 NetStreamDecoder::Read(i16* out, u32 frames)
    {
        if(m_Channels == 0)
            return 0;
        std::lock_guard<std::mutex> lock(m_Lock);
        const u64 cap   = (u64)m_RingFrames * m_Channels;
        u64 avail       = (m_RingWrite - m_RingRead) / m_Channels;
        u32 n           = (u32)Maths::Min<u64>(frames, avail);
        for(u32 i = 0; i < n * m_Channels; i++)
            out[i] = m_Ring[(size_t)((m_RingRead + i) % cap)];
        m_RingRead += (u64)n * m_Channels;
        if(n < frames)
        {
            if(m_State.load() == State::Ended || m_State.load() == State::Failed)
                return n; // really over
            // network behind: keep the stream alive with silence
            memset(out + n * m_Channels, 0, (size_t)(frames - n) * m_Channels * sizeof(i16));
            m_Starved = true;
            return frames;
        }
        m_Starved = false;
        return n;
    }
}
