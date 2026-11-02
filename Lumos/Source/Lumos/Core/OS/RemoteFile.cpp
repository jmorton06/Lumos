#include "Precompiled.h"
#include "RemoteFile.h"
#include "Core/OS/HttpStream.h"
#include "Maths/MathsUtilities.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace Lumos
{
    namespace
    {
        constexpr u64 NoChunk     = ~0ull;
        constexpr u64 JumpAhead   = 2 * 1024 * 1024; // a read further past the download than this restarts it there
        constexpr u32 TimeoutMs   = 20000;

        bool Seek(FILE* f, u64 offset)
        {
#if defined(_WIN32)
            return _fseeki64(f, (long long)offset, SEEK_SET) == 0;
#else
            return fseeko(f, (off_t)offset, SEEK_SET) == 0;
#endif
        }

        u64 FileSize(FILE* f)
        {
#if defined(_WIN32)
            _fseeki64(f, 0, SEEK_END);
            return (u64)_ftelli64(f);
#else
            fseeko(f, 0, SEEK_END);
            return (u64)ftello(f);
#endif
        }
    }

    RemoteFile::RemoteFile(const std::string& url, const std::string& cachePath, const Http::Headers& headers)
        : m_Url(url)
        , m_CachePath(cachePath)
        , m_PartPath(cachePath + ".part")
        , m_Headers(headers)
    {
        // already fetched on an earlier run
        if(FILE* f = fopen(cachePath.c_str(), "rb"))
        {
            u64 size = FileSize(f);
            if(size > 0)
            {
                m_File      = f;
                m_SizeKnown = true;
                m_Total     = size;
                m_Have      = size;
                m_Chunks.Resize((size_t)((size + ChunkSize - 1) / ChunkSize), 1);
                m_State = State::Complete;
                return;
            }
            fclose(f);
        }
        m_Thread = std::thread([this]() { Run(); });
    }

    RemoteFile::~RemoteFile()
    {
        Stop();
        if(m_Thread.joinable())
            m_Thread.join();
        if(m_File)
            fclose(m_File);
        if(m_State.load() != State::Complete)
            remove(m_PartPath.c_str());
    }

    void RemoteFile::Stop()
    {
        m_Stop = true;
        std::lock_guard<std::mutex> lock(m_Lock);
        m_Cv.notify_all();
    }

    std::string RemoteFile::GetError() const
    {
        std::lock_guard<std::mutex> lock(m_Lock);
        return m_Error;
    }

    std::string RemoteFile::GetContentType() const
    {
        std::lock_guard<std::mutex> lock(m_Lock);
        return m_ContentType;
    }

    void RemoteFile::Fail(const std::string& error)
    {
        std::lock_guard<std::mutex> lock(m_Lock);
        m_Error = error;
        m_State = State::Failed;
        m_Cv.notify_all();
    }

    void RemoteFile::Finish()
    {
        std::lock_guard<std::mutex> lock(m_Lock);
        fflush(m_File);
        remove(m_CachePath.c_str());
        if(rename(m_PartPath.c_str(), m_CachePath.c_str()) != 0)
            LWARN("RemoteFile: couldn't move %s into place", m_PartPath.c_str());
        m_State = State::Complete;
        m_Cv.notify_all();
    }

    u64 RemoteFile::NextMissing(u64 from) const
    {
        const u64 n = m_Chunks.Size();
        for(u64 i = 0; i < n; i++)
        {
            u64 c = (from + i) % n;
            if(!m_Chunks[c])
                return c;
        }
        return NoChunk;
    }

    bool RemoteFile::RangeReady(u64 offset, size_t bytes) const
    {
        if(bytes == 0)
            return true;
        u64 last = (offset + bytes - 1) / ChunkSize;
        for(u64 c = offset / ChunkSize; c <= last; c++)
            if(!ChunkReady(c))
                return false;
        return true;
    }

    void RemoteFile::Wake()
    {
        std::lock_guard<std::mutex> lock(m_Lock);
        m_Cv.notify_all();
    }

    u64 RemoteFile::Size(const std::atomic<bool>* cancel)
    {
        std::unique_lock<std::mutex> lock(m_Lock);
        m_Cv.wait(lock, [&] { return m_SizeKnown || m_State.load() == State::Failed || m_Stop.load() || (cancel && cancel->load()); });
        return m_SizeKnown ? m_Total.load() : 0;
    }

    size_t RemoteFile::Read(u64 offset, void* dst, size_t bytes, const std::atomic<bool>* cancel)
    {
        auto gaveUp = [&] { return m_State.load() == State::Failed || m_Stop.load() || (cancel && cancel->load()); };
        std::unique_lock<std::mutex> lock(m_Lock);
        m_Cv.wait(lock, [&] { return m_SizeKnown || gaveUp(); });
        if(!m_SizeKnown || offset >= m_Total.load())
            return 0;
        bytes = (size_t)Maths::Min<u64>(bytes, m_Total.load() - offset);
        if(!RangeReady(offset, bytes))
        {
            m_Want      = offset;
            m_WantMoved = true;
            m_Cv.wait(lock, [&] { return RangeReady(offset, bytes) || gaveUp(); });
            if(!RangeReady(offset, bytes))
                return 0;
        }
        if(!Seek(m_File, offset))
            return 0;
        return fread(dst, 1, bytes, m_File);
    }

    void RemoteFile::Run()
    {
        m_File = fopen(m_PartPath.c_str(), "w+b");
        if(!m_File)
        {
            Fail("can't write " + m_PartPath);
            return;
        }
        u32 failures = 0;
        while(!m_Stop.load())
        {
            u64 chunk = 0;
            {
                std::lock_guard<std::mutex> lock(m_Lock);
                if(m_SizeKnown)
                {
                    chunk = m_NoRanges ? 0 : NextMissing(m_Want / ChunkSize);
                    m_WantMoved = false;
                }
            }
            if(chunk == NoChunk)
            {
                Finish();
                return;
            }
            if(Fetch(chunk * ChunkSize))
            {
                failures = 0;
                continue;
            }
            if(m_Stop.load() || m_State.load() == State::Failed)
                return;
            if(++failures >= 4)
            {
                Fail(m_Error.empty() ? "connection lost" : m_Error);
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(400 * failures));
        }
    }

    bool RemoteFile::Fetch(u64 start)
    {
        Http::Headers headers = m_Headers;
        headers.push_back({ "Range", "bytes=" + std::to_string(start) + "-" });
        HttpStream* http = HttpStream::Open(m_Url, headers);
        auto began       = std::chrono::steady_clock::now();
        while(!http->HasResponse() && !m_Stop.load())
        {
            if(std::chrono::steady_clock::now() - began > std::chrono::milliseconds(TimeoutMs))
                break;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        int status = http->GetStatus();
        if(m_Stop.load() || (status != 200 && status != 206))
        {
            std::string err = http->GetError();
            if(!m_Stop.load())
            {
                std::lock_guard<std::mutex> lock(m_Lock);
                m_Error = !err.empty() ? err : (status ? "HTTP " + std::to_string(status) : std::string("no answer"));
            }
            delete http;
            // a 4xx won't get better by asking again
            if(status >= 400 && status < 500)
                Fail(m_Error);
            return false;
        }

        u64 pos = start;
        {
            std::lock_guard<std::mutex> lock(m_Lock);
            m_ContentType = http->GetHeader("content-type");
            u64 total     = 0;
            if(status == 206)
            {
                // bytes a-b/total
                std::string range = http->GetHeader("content-range");
                size_t slash      = range.rfind('/');
                if(slash != std::string::npos && range[slash + 1] != '*')
                    total = strtoull(range.c_str() + slash + 1, nullptr, 10);
            }
            else
            {
                std::string len = http->GetHeader("content-length");
                total           = len.empty() ? 0 : strtoull(len.c_str(), nullptr, 10);
                m_NoRanges      = true; // the whole file, from the top
                pos             = 0;
            }
            if(!m_SizeKnown)
            {
                if(total == 0)
                {
                    m_Error = "the server didn't say how big the file is";
                    delete http;
                    m_State = State::Failed;
                    m_Cv.notify_all();
                    return false;
                }
                m_Total     = total;
                m_SizeKnown = true;
                m_Chunks.Resize((size_t)((total + ChunkSize - 1) / ChunkSize), 0);
                m_State = State::Downloading;
                m_Cv.notify_all();
            }
        }

        const u64 total = m_Total.load();
        const u64 begin = pos;
        static constexpr size_t BufSize = 64 * 1024;
        u8* buf = new u8[BufSize];
        bool restart = false;
        while(!m_Stop.load() && pos < total)
        {
            size_t n = http->Read(buf, BufSize, 500);
            if(n == 0)
            {
                if(http->IsFinished())
                    break;
                continue;
            }
            n = (size_t)Maths::Min<u64>(n, total - pos);
            std::lock_guard<std::mutex> lock(m_Lock);
            if(!Seek(m_File, pos) || fwrite(buf, 1, n, m_File) != n)
            {
                m_Error = "disk full?";
                break;
            }
            u64 end = pos + n;
            // chunks this connection has now covered whole
            for(u64 c = pos / ChunkSize; c < m_Chunks.Size(); c++)
            {
                u64 cs = c * ChunkSize, ce = Maths::Min<u64>(cs + ChunkSize, total);
                if(cs < begin || ce > end)
                    break;
                if(!m_Chunks[c])
                {
                    m_Chunks[c] = 1;
                    m_Have      = m_Have.load() + (ce - cs);
                }
            }
            if(pos / ChunkSize != end / ChunkSize || end == total)
                m_Cv.notify_all();
            pos = end;
            if(m_NoRanges)
                continue;
            // a reader wants somewhere this connection won't reach soon
            if(m_WantMoved)
            {
                u64 wc = m_Want / ChunkSize;
                if(!ChunkReady(wc) && (wc * ChunkSize < begin || wc * ChunkSize > pos + JumpAhead))
                {
                    restart = true;
                    break;
                }
                m_WantMoved = false;
            }
            // ran into bytes already here
            if(pos % ChunkSize == 0 && ChunkReady(pos / ChunkSize))
            {
                restart = true;
                break;
            }
        }
        delete[] buf;
        std::string err = http->GetError();
        delete http;
        if(restart || pos >= total)
            return true;
        if(!err.empty())
        {
            std::lock_guard<std::mutex> lock(m_Lock);
            m_Error = err;
        }
        return pos > begin;
    }
}
