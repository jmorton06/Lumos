#include "Precompiled.h"
#include "RemoteAudio.h"
#include "AudioDecoder.h"
#include "Core/OS/RemoteFile.h"
#include "Core/DataStructures/TDArray.h"
#include "Maths/MathsUtilities.h"
#include "Utilities/StringUtilities.h"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>

namespace Lumos
{
    namespace
    {
        using FilePtr = std::shared_ptr<RemoteFile>;

        size_t SourceRead(void* user, u64 offset, void* dst, size_t bytes);
        u64 SourceSize(void* user);

        // Decodes on its own thread into a few seconds of float PCM. Read never blocks.
        class RemoteDecoder : public AudioDecoder
        {
        public:
            enum class State
            {
                Opening,
                Ready,
                Failed,
            };

            RemoteDecoder(FilePtr file, const std::string& ext)
                : m_File(std::move(file))
                , m_Ext(ext)
            {
                m_Thread = std::thread([this]() { Run(); });
            }

            ~RemoteDecoder() override
            {
                m_Stop = true;
                m_File->Wake();
                if(m_Thread.joinable())
                    m_Thread.join();
                delete m_Inner;
            }

            State GetState() const { return m_State.load(); }

            bool WaitReady(u32 timeoutMs)
            {
                auto start = std::chrono::steady_clock::now();
                while(m_State.load() == State::Opening)
                {
                    if(std::chrono::steady_clock::now() - start > std::chrono::milliseconds(timeoutMs))
                        return false;
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                }
                return m_State.load() == State::Ready;
            }

            bool IsBuffering() const override { return m_Starved.load(); }

            u32 ReadFloat(float* out, u32 frames) override
            {
                if(m_Channels == 0)
                    return 0;
                std::lock_guard<std::mutex> lock(m_Lock);
                const u64 cap = (u64)m_RingFrames * m_Channels;
                u64 avail     = (m_RingWrite - m_RingRead) / m_Channels;
                u32 n         = m_SeekPending ? 0 : (u32)Maths::Min<u64>(frames, avail);
                for(u32 i = 0; i < n * m_Channels; i++)
                    out[i] = m_Ring[(size_t)((m_RingRead + i) % cap)];
                m_RingRead += (u64)n * m_Channels;
                if(n < frames)
                {
                    if(m_Ended && !m_SeekPending)
                        return n;
                    // the download is behind: silence until it catches up
                    memset(out + (size_t)n * m_Channels, 0, (size_t)(frames - n) * m_Channels * sizeof(float));
                    m_Starved = true;
                    return frames;
                }
                m_Starved = false;
                return n;
            }

            u32 Read(i16* out, u32 frames) override
            {
                m_Tmp.Resize((size_t)frames * Maths::Max(1u, m_Channels));
                u32 n = ReadFloat(m_Tmp.Data(), frames);
                for(u32 i = 0; i < n * m_Channels; i++)
                {
                    float v = Maths::Clamp(m_Tmp[i], -1.0f, 1.0f);
                    out[i]  = (i16)(v * 32767.0f);
                }
                return n;
            }

            bool Seek(u64 frame) override
            {
                std::lock_guard<std::mutex> lock(m_Lock);
                m_SeekTo      = frame;
                m_SeekPending = true;
                m_Generation++;
                m_RingRead = m_RingWrite;
                m_Ended    = false;
                return true;
            }

            size_t ReadBytes(u64 offset, void* dst, size_t bytes) { return m_File->Read(offset, dst, bytes, &m_Stop); }
            u64 TotalBytes() { return m_File->Size(&m_Stop); }

        private:
            void Run()
            {
                AudioByteSource src;
                src.user = this;
                src.read = SourceRead;
                src.size = SourceSize;
                m_Inner  = AudioDecoder::OpenSource(src, m_Ext);
                if(!m_Inner || m_Stop.load())
                {
                    if(!m_Stop.load())
                        LWARN("RemoteAudio: can't decode %s (%s)", m_File->GetUrl().c_str(), m_File->GetError().c_str());
                    m_State = State::Failed;
                    return;
                }
                m_SampleRate  = m_Inner->GetSampleRate();
                m_Channels    = m_Inner->GetChannels();
                m_TotalFrames = m_Inner->GetTotalFrames();
                m_Codec       = m_Inner->GetCodec();
                m_Lossless    = m_Inner->IsLossless();
                m_SourceBits  = m_Inner->GetSourceBits();
                m_FloatSource = m_Inner->IsFloatSource();
                m_RingFrames  = m_SampleRate * 10;
                {
                    std::lock_guard<std::mutex> lock(m_Lock);
                    m_Ring.Resize((size_t)m_RingFrames * m_Channels, 0.0f);
                }

                const u32 Block    = 2048;
                const u32 prebuffer = m_SampleRate / 2;
                TDArray<float> buf;
                buf.Resize((size_t)Block * m_Channels);
                while(!m_Stop.load())
                {
                    u64 seekTo = 0, gen = 0;
                    bool seek = false, full = false;
                    {
                        std::lock_guard<std::mutex> lock(m_Lock);
                        seek   = m_SeekPending;
                        seekTo = m_SeekTo;
                        gen    = m_Generation;
                        full   = (m_RingWrite - m_RingRead) / m_Channels + Block > m_RingFrames;
                        if(m_Ended && !seek)
                            full = true;
                    }
                    if(seek)
                    {
                        m_Inner->Seek(seekTo);
                        std::lock_guard<std::mutex> lock(m_Lock);
                        if(gen == m_Generation)
                        {
                            m_SeekPending = false;
                            m_RingRead    = m_RingWrite;
                        }
                        continue;
                    }
                    if(full)
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                        continue;
                    }
                    u32 n = m_Inner->ReadFloat(buf.Data(), Block);
                    std::lock_guard<std::mutex> lock(m_Lock);
                    if(gen != m_Generation)
                        continue; // a seek came in while that was decoding
                    if(n == 0)
                    {
                        m_Ended = true;
                        if(m_State.load() == State::Opening)
                            m_State = State::Ready; // shorter than the prebuffer
                        continue;
                    }
                    const u64 cap = (u64)m_RingFrames * m_Channels;
                    for(u32 i = 0; i < n * m_Channels; i++)
                        m_Ring[(size_t)((m_RingWrite + i) % cap)] = buf[i];
                    m_RingWrite += (u64)n * m_Channels;
                    if(m_State.load() == State::Opening && (m_RingWrite - m_RingRead) / m_Channels >= prebuffer)
                        m_State = State::Ready;
                }
            }

            FilePtr m_File;
            std::string m_Ext;
            AudioDecoder* m_Inner = nullptr;
            std::thread m_Thread;
            std::atomic<bool> m_Stop { false };
            std::atomic<State> m_State { State::Opening };
            std::atomic<bool> m_Starved { false };

            std::mutex m_Lock;
            TDArray<float> m_Ring; // interleaved
            u64 m_RingRead = 0, m_RingWrite = 0; // in samples
            u32 m_RingFrames   = 0;
            bool m_Ended       = false;
            bool m_SeekPending = false;
            u64 m_SeekTo = 0, m_Generation = 0;
            TDArray<float> m_Tmp; // Read's conversion buffer (audio thread only)
        };

        size_t SourceRead(void* user, u64 offset, void* dst, size_t bytes) { return ((RemoteDecoder*)user)->ReadBytes(offset, dst, bytes); }
        u64 SourceSize(void* user) { return ((RemoteDecoder*)user)->TotalBytes(); }

        struct Entry
        {
            FilePtr file;
            RemoteDecoder* prepared = nullptr; // waiting for a stream to take it
            std::string ext;
        };

        std::mutex s_Lock;
        std::unordered_map<std::string, Entry> s_Entries;

        std::string ExtOf(const std::string& path)
        {
            std::string p = path.substr(0, path.find('?'));
            size_t slash  = p.find_last_of("/\\");
            size_t dot    = p.find_last_of('.');
            if(dot == std::string::npos || (slash != std::string::npos && dot < slash))
                return {};
            return StringUtilities::ToLower(p.substr(dot + 1));
        }

        std::string TempPathFor(const std::string& url)
        {
            const char* tmp = getenv("TMPDIR");
            std::string dir = tmp && *tmp ? tmp : "/tmp";
            if(dir.back() != '/')
                dir += '/';
            u64 h = 1469598103934665603ull;
            for(char c : url)
                h = (h ^ (u8)c) * 1099511628211ull;
            std::string ext = ExtOf(url);
            return dir + "lumos-remote-" + std::to_string(h) + (ext.empty() ? "" : "." + ext);
        }
    }

    bool RemoteAudio::IsUrl(const std::string& path)
    {
        return path.compare(0, 7, "http://") == 0 || path.compare(0, 8, "https://") == 0;
    }

    void RemoteAudio::Prepare(const std::string& url, const std::string& cachePath, const Http::Headers& headers, bool decode)
    {
        std::lock_guard<std::mutex> lock(s_Lock);
        Entry& e = s_Entries[url];
        if(!e.file)
        {
            e.ext  = ExtOf(cachePath);
            e.file = std::make_shared<RemoteFile>(url, cachePath, headers);
        }
        if(decode && !e.prepared && e.file->GetState() != RemoteFile::State::Complete)
            e.prepared = new RemoteDecoder(e.file, e.ext);
    }

    RemoteAudio::Status RemoteAudio::GetStatus(const std::string& url)
    {
        Status st;
        std::lock_guard<std::mutex> lock(s_Lock);
        auto it = s_Entries.find(url);
        if(it == s_Entries.end())
            return st;
        Entry& e    = it->second;
        auto fs     = e.file->GetState();
        st.have     = e.file->GetHave();
        st.total    = e.file->GetTotal();
        st.complete = fs == RemoteFile::State::Complete;
        st.error    = e.file->GetError();
        if(fs == RemoteFile::State::Failed || (e.prepared && e.prepared->GetState() == RemoteDecoder::State::Failed))
            st.state = "failed";
        else if(e.prepared)
            st.state = e.prepared->GetState() == RemoteDecoder::State::Ready ? "ready" : "opening";
        else
            st.state = st.complete ? "complete" : "downloading";
        return st;
    }

    void RemoteAudio::Release(const std::string& url)
    {
        RemoteDecoder* prepared = nullptr;
        {
            std::lock_guard<std::mutex> lock(s_Lock);
            auto it = s_Entries.find(url);
            if(it == s_Entries.end())
                return;
            prepared = it->second.prepared;
            s_Entries.erase(it);
        }
        delete prepared; // joins its thread: outside the lock
    }

    AudioDecoder* RemoteAudio::Open(const std::string& url, bool precise)
    {
        RemoteDecoder* dec = nullptr;
        FilePtr file;
        std::string ext;
        {
            std::lock_guard<std::mutex> lock(s_Lock);
            auto it = s_Entries.find(url);
            if(it == s_Entries.end())
            {
                Entry& e = s_Entries[url];
                std::string path = TempPathFor(url);
                e.ext  = ExtOf(path);
                e.file = std::make_shared<RemoteFile>(url, path);
                it     = s_Entries.find(url);
            }
            Entry& e = it->second;
            file     = e.file;
            ext      = e.ext;
            dec      = e.prepared;
            e.prepared = nullptr;
        }
        // whole already: a plain file decoder
        if(file->GetState() == RemoteFile::State::Complete)
        {
            delete dec;
            const std::string& path = file->GetCachePath();
            return precise ? AudioDecoder::OpenPrecise(path) : AudioDecoder::Open(path);
        }
        if(!dec)
            dec = new RemoteDecoder(file, ext);
        if(dec->WaitReady(20000))
            return dec;
        LWARN("RemoteAudio: %s didn't open (%s)", url.c_str(), file->GetError().c_str());
        delete dec;
        return nullptr;
    }

    void RemoteAudio::ShutdownAll()
    {
        std::unordered_map<std::string, Entry> entries;
        {
            std::lock_guard<std::mutex> lock(s_Lock);
            entries.swap(s_Entries);
        }
        for(auto& kv : entries)
        {
            kv.second.file->Stop();
            delete kv.second.prepared;
        }
    }
}
