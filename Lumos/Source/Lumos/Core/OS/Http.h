#pragma once
#include "Core/Core.h"
#include <string>
#include <utility>
#include <vector>

namespace Lumos
{
    // Minimal async HTTP GET / POST using the platform's own stack (NSURLSession / WinHTTP /
    // curl on Linux). Redirects are followed. Poll from the main thread.
    namespace Http
    {
        using Headers = std::vector<std::pair<std::string, std::string>>;

        struct Response
        {
            int Status = 0;      // HTTP status, 0 if the request never got a response
            std::string Body;    // raw bytes
            std::string Error;   // transport error, empty on success
        };

        u32 Get(const std::string& url, const Headers& headers = {});
        // Sends `body` as it is; set Content-Type in the headers. Not on Windows yet.
        u32 Post(const std::string& url, const std::string& body, const Headers& headers = {});
        // GET straight into a file (big downloads never sit in memory); the response body stays empty.
        // Apple platforms and Linux.
        u32 Download(const std::string& url, const std::string& path, const Headers& headers = {});
        // True once the request has finished; fills `out` and forgets the request.
        bool Poll(u32 id, Response& out);
        void Cancel(u32 id);
        u32 PendingCount();

        std::string UrlEncode(const std::string& s);

        namespace Detail
        {
            // Called by the platform backend from any thread
            void Complete(u32 id, int status, std::string&& body, std::string&& error);
            void PlatformGet(u32 id, const std::string& url, const Headers& headers);
            void PlatformPost(u32 id, const std::string& url, const std::string& body, const Headers& headers);
            void PlatformDownload(u32 id, const std::string& url, const std::string& path, const Headers& headers);
        }
    }
}
