#include "Precompiled.h"
#include "HttpStream.h"
#include <algorithm>
#include <chrono>

namespace Lumos
{
    namespace
    {
        constexpr size_t MaxBuffered = 4 * 1024 * 1024;
    }

    HttpStream* HttpStream::Open(const std::string& url, const Http::Headers& headers)
    {
        HttpStream* s = new HttpStream();
        HttpStreamPlatform::Start(s, url, headers);
        return s;
    }

    HttpStream::~HttpStream()
    {
        HttpStreamPlatform::Stop(this);
    }

    void HttpStream::OnResponse(int status, Http::Headers&& headers)
    {
        std::lock_guard<std::mutex> lock(m_Lock);
        m_Headers = std::move(headers);
        for(auto& h : m_Headers)
            std::transform(h.first.begin(), h.first.end(), h.first.begin(), ::tolower);
        m_Status      = status;
        m_HasResponse = true;
        m_Cv.notify_all();
    }

    void HttpStream::OnData(const u8* data, size_t size)
    {
        std::lock_guard<std::mutex> lock(m_Lock);
        // compact now and then instead of erasing from the front every read
        if(m_Head > 0 && m_Head >= m_Buffer.size() / 2)
        {
            m_Buffer.erase(0, m_Head);
            m_Head = 0;
        }
        m_Buffer.append((const char*)data, size);
        if(m_Buffer.size() - m_Head > MaxBuffered)
            m_Head = m_Buffer.size() - MaxBuffered;
        m_Cv.notify_all();
    }

    void HttpStream::OnComplete(const std::string& error)
    {
        std::lock_guard<std::mutex> lock(m_Lock);
        m_Done  = true;
        m_Error = error;
        m_HasResponse = true;
        m_Cv.notify_all();
    }

    size_t HttpStream::Read(u8* dst, size_t max, u32 timeoutMs)
    {
        std::unique_lock<std::mutex> lock(m_Lock);
        m_Cv.wait_for(lock, std::chrono::milliseconds(timeoutMs), [&] { return m_Buffer.size() > m_Head || m_Done; });
        size_t n = std::min(max, m_Buffer.size() - m_Head);
        memcpy(dst, m_Buffer.data() + m_Head, n);
        m_Head += n;
        return n;
    }

    bool HttpStream::IsFinished() const
    {
        std::lock_guard<std::mutex> lock(m_Lock);
        return m_Done && m_Head >= m_Buffer.size();
    }

    std::string HttpStream::GetHeader(const std::string& lowerName) const
    {
        std::lock_guard<std::mutex> lock(m_Lock);
        for(auto& h : m_Headers)
            if(h.first == lowerName)
                return h.second;
        return {};
    }

    std::string HttpStream::GetError() const
    {
        std::lock_guard<std::mutex> lock(m_Lock);
        return m_Error;
    }

#if !defined(LUMOS_PLATFORM_MACOS) && !defined(LUMOS_PLATFORM_IOS)
    // TODO: WinHTTP / curl streaming backends
    void HttpStreamPlatform::Start(HttpStream* stream, const std::string&, const Http::Headers&)
    {
        stream->OnComplete("streaming HTTP isn't supported on this platform yet");
    }
    void HttpStreamPlatform::Stop(HttpStream*) { }
#endif
}
