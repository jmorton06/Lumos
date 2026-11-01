#pragma once
#include "Core/Core.h"
#include "Core/OS/Http.h"
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>

namespace Lumos
{
    // A GET whose body arrives as a stream (internet radio). Connects in the background; Read blocks
    // for up to a timeout. Keeps at most a few MB buffered - a reader that stops reading loses the oldest.
    class LUMOS_EXPORT HttpStream
    {
    public:
        static HttpStream* Open(const std::string& url, const Http::Headers& headers = {});
        ~HttpStream();

        // Bytes read (0: nothing arrived within the timeout, or the stream is over - check IsFinished)
        size_t Read(u8* dst, size_t max, u32 timeoutMs);
        bool IsFinished() const;       // body complete (or failed) and fully read
        bool HasResponse() const { return m_HasResponse.load(); }
        int GetStatus() const { return m_Status.load(); }
        std::string GetHeader(const std::string& lowerName) const;
        std::string GetError() const;

        // ---- platform backend ----
        void OnResponse(int status, Http::Headers&& headers);
        void OnData(const u8* data, size_t size);
        void OnComplete(const std::string& error);
        void* Platform = nullptr;

    private:
        HttpStream() = default;

        mutable std::mutex m_Lock;
        std::condition_variable m_Cv;
        std::string m_Buffer; // unread bytes from m_Head
        size_t m_Head = 0;
        bool m_Done   = false;
        std::string m_Error;
        Http::Headers m_Headers;
        std::atomic<int> m_Status { 0 };
        std::atomic<bool> m_HasResponse { false };
    };

    namespace HttpStreamPlatform
    {
        void Start(HttpStream* stream, const std::string& url, const Http::Headers& headers);
        void Stop(HttpStream* stream);
    }
}
