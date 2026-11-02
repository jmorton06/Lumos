#pragma once
#include "Core/Core.h"
#include "Core/OS/Http.h"
#include "Core/DataStructures/TDArray.h"
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

namespace Lumos
{
    class HttpStream;

    // A file on a web server read like a local one. Downloads into `cachePath` (as `.part` until
    // whole) on its own thread with Range requests; a read of bytes not there yet waits for them, and
    // one far from the download moves it there. Once every byte is in, the part file becomes cachePath.
    class LUMOS_EXPORT RemoteFile
    {
    public:
        enum class State
        {
            Connecting,
            Downloading,
            Complete,
            Failed,
        };

        RemoteFile(const std::string& url, const std::string& cachePath, const Http::Headers& headers = {});
        ~RemoteFile();

        // Waits for the size (0 = failed or unknown). `cancel` lets one reader give up without
        // stopping the download (set it, then Wake).
        u64 Size(const std::atomic<bool>* cancel = nullptr);
        // Waits for the bytes; returns fewer only at the end of the file, on failure or when cancelled
        size_t Read(u64 offset, void* dst, size_t bytes, const std::atomic<bool>* cancel = nullptr);
        void Wake();
        void Stop();

        State GetState() const { return m_State.load(); }
        std::string GetError() const;
        u64 GetHave() const { return m_Have.load(); }
        u64 GetTotal() const { return m_Total.load(); }
        const std::string& GetUrl() const { return m_Url; }
        const std::string& GetCachePath() const { return m_CachePath; }
        std::string GetContentType() const;

    private:
        void Run();
        bool Fetch(u64 start); // one connection; false = give up on it
        void Fail(const std::string& error);
        void Finish();
        bool ChunkReady(u64 chunk) const { return chunk < m_Chunks.Size() && m_Chunks[chunk] != 0; }
        u64 NextMissing(u64 fromChunk) const; // ~0 when every chunk is in
        bool RangeReady(u64 offset, size_t bytes) const;

        static constexpr u64 ChunkSize = 128 * 1024;

        std::string m_Url, m_CachePath, m_PartPath, m_ContentType, m_Error;
        Http::Headers m_Headers;
        std::thread m_Thread;
        std::atomic<bool> m_Stop { false };
        std::atomic<State> m_State { State::Connecting };
        std::atomic<u64> m_Have { 0 }, m_Total { 0 };
        bool m_SizeKnown  = false;
        bool m_NoRanges   = false; // server answered a Range request with the whole file
        u64 m_Want        = 0;     // where the last waiting read is (byte offset)
        bool m_WantMoved  = false;

        mutable std::mutex m_Lock;
        std::condition_variable m_Cv;
        TDArray<u8> m_Chunks; // 1 = downloaded
        FILE* m_File = nullptr;
    };
}
