#include "Precompiled.h"
#include "Http.h"
#include "Core/Mutex.h"

#include <unordered_map>
#include <thread>

#if defined(LUMOS_PLATFORM_WINDOWS)
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#elif defined(LUMOS_PLATFORM_LINUX)
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdio>
extern char** environ;
#endif

namespace Lumos
{
    namespace Http
    {
        namespace
        {
            struct Entry
            {
                bool Done = false;
                bool Cancelled = false;
                Response Result;
            };

            struct State
            {
                Mutex Lock;
                std::unordered_map<u32, Entry> Requests;
                u32 NextId = 1;
                State() { MutexInit(&Lock); }
            };

            State& GetState()
            {
                static State s_State;
                return s_State;
            }
        }

        void Detail::Complete(u32 id, int status, std::string&& body, std::string&& error)
        {
            auto& st = GetState();
            ScopedMutex lock(&st.Lock);
            auto it = st.Requests.find(id);
            if(it == st.Requests.end())
                return;
            if(it->second.Cancelled)
            {
                st.Requests.erase(it);
                return;
            }
            it->second.Done          = true;
            it->second.Result.Status = status;
            it->second.Result.Body   = std::move(body);
            it->second.Result.Error  = std::move(error);
        }

        u32 Get(const std::string& url, const Headers& headers)
        {
            auto& st = GetState();
            u32 id;
            {
                ScopedMutex lock(&st.Lock);
                id = st.NextId++;
                st.Requests[id] = Entry();
            }
            Detail::PlatformGet(id, url, headers);
            return id;
        }

        u32 Post(const std::string& url, const std::string& body, const Headers& headers)
        {
            auto& st = GetState();
            u32 id;
            {
                ScopedMutex lock(&st.Lock);
                id = st.NextId++;
                st.Requests[id] = Entry();
            }
            Detail::PlatformPost(id, url, body, headers);
            return id;
        }

        u32 Download(const std::string& url, const std::string& path, const Headers& headers)
        {
            auto& st = GetState();
            u32 id;
            {
                ScopedMutex lock(&st.Lock);
                id = st.NextId++;
                st.Requests[id] = Entry();
            }
            Detail::PlatformDownload(id, url, path, headers);
            return id;
        }

        bool Poll(u32 id, Response& out)
        {
            auto& st = GetState();
            ScopedMutex lock(&st.Lock);
            auto it = st.Requests.find(id);
            if(it == st.Requests.end())
            {
                out       = Response();
                out.Error = "unknown request";
                return true;
            }
            if(!it->second.Done)
                return false;
            out = std::move(it->second.Result);
            st.Requests.erase(it);
            return true;
        }

        void Cancel(u32 id)
        {
            auto& st = GetState();
            ScopedMutex lock(&st.Lock);
            auto it = st.Requests.find(id);
            if(it == st.Requests.end())
                return;
            if(it->second.Done)
                st.Requests.erase(it);
            else
                it->second.Cancelled = true; // dropped when the backend completes
        }

        u32 PendingCount()
        {
            auto& st = GetState();
            ScopedMutex lock(&st.Lock);
            u32 n = 0;
            for(auto& kv : st.Requests)
                n += (!kv.second.Done && !kv.second.Cancelled) ? 1 : 0;
            return n;
        }

        std::string UrlEncode(const std::string& s)
        {
            static const char* hex = "0123456789ABCDEF";
            std::string out;
            for(unsigned char c : s)
            {
                if(isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
                    out += (char)c;
                else
                {
                    out += '%';
                    out += hex[c >> 4];
                    out += hex[c & 15];
                }
            }
            return out;
        }

#if defined(LUMOS_PLATFORM_WINDOWS)
        void Detail::PlatformGet(u32 id, const std::string& url, const Headers& headers)
        {
            std::thread([id, url, headers]()
                        {
                auto widen = [](const std::string& s) {
                    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
                    std::wstring w(n > 0 ? n - 1 : 0, L'\0');
                    if(n > 1)
                        MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &w[0], n);
                    return w;
                };
                std::wstring wurl = widen(url);
                URL_COMPONENTS uc = {};
                uc.dwStructSize   = sizeof(uc);
                wchar_t host[256] = {}, path[4096] = {}, extra[4096] = {};
                uc.lpszHostName     = host;
                uc.dwHostNameLength = 256;
                uc.lpszUrlPath      = path;
                uc.dwUrlPathLength  = 4096;
                uc.lpszExtraInfo    = extra;
                uc.dwExtraInfoLength = 4096;
                if(!WinHttpCrackUrl(wurl.c_str(), 0, 0, &uc))
                {
                    Detail::Complete(id, 0, std::string(), "bad url");
                    return;
                }
                std::wstring object = std::wstring(path) + extra;

                std::string body, error;
                DWORD status = 0;
                HINTERNET session = WinHttpOpen(L"Lumos", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
                HINTERNET connect = session ? WinHttpConnect(session, host, uc.nPort, 0) : nullptr;
                HINTERNET request = connect ? WinHttpOpenRequest(connect, L"GET", object.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                                                 uc.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0)
                                            : nullptr;
                if(request)
                {
                    for(auto& h : headers)
                    {
                        std::wstring line = widen(h.first + ": " + h.second);
                        WinHttpAddRequestHeaders(request, line.c_str(), (DWORD)-1L, WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);
                    }
                    if(WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) && WinHttpReceiveResponse(request, nullptr))
                    {
                        DWORD size = sizeof(status);
                        WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX);
                        DWORD avail = 0;
                        while(WinHttpQueryDataAvailable(request, &avail) && avail > 0)
                        {
                            size_t old = body.size();
                            body.resize(old + avail);
                            DWORD read = 0;
                            if(!WinHttpReadData(request, &body[old], avail, &read))
                                break;
                            body.resize(old + read);
                        }
                    }
                    else
                        error = "request failed";
                }
                else
                    error = "couldn't open connection";
                if(request)
                    WinHttpCloseHandle(request);
                if(connect)
                    WinHttpCloseHandle(connect);
                if(session)
                    WinHttpCloseHandle(session);
                Detail::Complete(id, (int)status, std::move(body), std::move(error)); })
                .detach();
        }
        // TODO: WinHTTP POST / download to file
        void Detail::PlatformPost(u32 id, const std::string&, const std::string&, const Headers&)
        {
            Detail::Complete(id, 0, std::string(), "HTTP POST not supported on this platform");
        }
        void Detail::PlatformDownload(u32 id, const std::string&, const std::string&, const Headers&)
        {
            Detail::Complete(id, 0, std::string(), "HTTP download not supported on this platform");
        }
#elif defined(LUMOS_PLATFORM_LINUX)
        namespace
        {
            void CurlRequest(u32 id, const std::string& url, const Headers& headers, bool post, const std::string& postBody, const std::string& toFile = std::string());
        }
        void Detail::PlatformDownload(u32 id, const std::string& url, const std::string& path, const Headers& headers) { CurlRequest(id, url, headers, false, std::string(), path); }
        void Detail::PlatformGet(u32 id, const std::string& url, const Headers& headers) { CurlRequest(id, url, headers, false, std::string()); }
        void Detail::PlatformPost(u32 id, const std::string& url, const std::string& body, const Headers& headers) { CurlRequest(id, url, headers, true, body); }

        namespace
        {
        // No libcurl link dependency: run the curl CLI (argv, no shell) and read its output.
        void CurlRequest(u32 id, const std::string& url, const Headers& headers, bool post, const std::string& postBody, const std::string& toFile)
        {
            std::thread([id, url, headers, post, postBody, toFile]()
                        {
                char tmpl[] = "/tmp/lumos_http_XXXXXX";
                int fd      = mkstemp(tmpl);
                if(fd < 0)
                {
                    Detail::Complete(id, 0, std::string(), "no temp file");
                    return;
                }
                close(fd);

                std::vector<std::string> args = { "curl", "-sSL", "--max-time", toFile.empty() ? "30" : "3600", "-o", toFile.empty() ? tmpl : toFile.c_str(), "-w", "%{http_code}" };
                for(auto& h : headers)
                {
                    args.push_back("-H");
                    args.push_back(h.first + ": " + h.second);
                }
                if(post)
                {
                    args.push_back("--data-binary");
                    args.push_back(postBody);
                }
                args.push_back(url);
                std::vector<char*> argv;
                for(auto& a : args)
                    argv.push_back((char*)a.c_str());
                argv.push_back(nullptr);

                int pipefd[2];
                if(pipe(pipefd) != 0)
                {
                    unlink(tmpl);
                    Detail::Complete(id, 0, std::string(), "pipe failed");
                    return;
                }
                posix_spawn_file_actions_t actions;
                posix_spawn_file_actions_init(&actions);
                posix_spawn_file_actions_adddup2(&actions, pipefd[1], STDOUT_FILENO);
                posix_spawn_file_actions_addclose(&actions, pipefd[0]);
                pid_t pid = 0;
                int rc    = posix_spawnp(&pid, "curl", &actions, nullptr, argv.data(), environ);
                posix_spawn_file_actions_destroy(&actions);
                close(pipefd[1]);

                std::string statusText, body, error;
                if(rc == 0)
                {
                    char buf[64];
                    ssize_t n;
                    while((n = read(pipefd[0], buf, sizeof(buf))) > 0)
                        statusText.append(buf, (size_t)n);
                    int wstatus = 0;
                    waitpid(pid, &wstatus, 0);
                    if(!(WIFEXITED(wstatus) && WEXITSTATUS(wstatus) == 0))
                        error = "curl failed";
                    if(FILE* f = toFile.empty() ? fopen(tmpl, "rb") : nullptr)
                    {
                        char chunk[16384];
                        size_t r;
                        while((r = fread(chunk, 1, sizeof(chunk), f)) > 0)
                            body.append(chunk, r);
                        fclose(f);
                    }
                }
                else
                    error = "curl not found";
                close(pipefd[0]);
                unlink(tmpl);
                Detail::Complete(id, atoi(statusText.c_str()), std::move(body), std::move(error)); })
                .detach();
        }
        }
#elif !defined(LUMOS_PLATFORM_MACOS) && !defined(LUMOS_PLATFORM_IOS)
        // macOS / iOS live in Platform/MacOS/MacOSHttp.mm (Foundation)
        void Detail::PlatformGet(u32 id, const std::string&, const Headers&)
        {
            Detail::Complete(id, 0, std::string(), "HTTP not supported on this platform");
        }
        void Detail::PlatformPost(u32 id, const std::string&, const std::string&, const Headers&)
        {
            Detail::Complete(id, 0, std::string(), "HTTP not supported on this platform");
        }
        void Detail::PlatformDownload(u32 id, const std::string&, const std::string&, const Headers&)
        {
            Detail::Complete(id, 0, std::string(), "HTTP not supported on this platform");
        }
#endif
    }
}
