#include "Precompiled.h"
#include "AsyncFileList.h"
#include "Utilities/StringUtilities.h"

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <mutex>
#include <thread>

namespace fs = std::filesystem;

namespace Lumos
{
    namespace AsyncFileList
    {
        namespace
        {
            struct Job
            {
                u32 id = 0;
                std::thread thread;
                std::atomic<bool> done { false };
                TDArray<std::string> files;
            };

            std::mutex s_Lock;
            TDArray<Job*> s_Jobs;
            u32 s_NextId = 1;
        }

        void ParseExtensions(const std::string& csv, TDArray<std::string>& out)
        {
            std::string cur;
            for(char c : csv + ",")
            {
                if(c == ',' || c == ';' || c == ' ')
                {
                    if(!cur.empty())
                        out.PushBack(StringUtilities::ToLower(cur));
                    cur.clear();
                }
                else if(c != '.' && c != '*')
                    cur += c;
            }
        }

        void List(const std::string& rootPath, bool recursive, const TDArray<std::string>& exts, TDArray<std::string>& out)
        {
            auto accept = [&](const fs::path& p)
            {
                if(exts.Empty())
                    return true;
                std::string ext = p.extension().string();
                if(!ext.empty() && ext[0] == '.')
                    ext = ext.substr(1);
                ext = StringUtilities::ToLower(ext);
                for(u32 i = 0; i < exts.Size(); i++)
                    if(exts[i] == ext)
                        return true;
                return false;
            };

            std::error_code ec;
            fs::path root(rootPath);
            if(!fs::is_directory(root, ec))
                return;

            auto opts = fs::directory_options::skip_permission_denied;
            if(recursive)
            {
                for(fs::recursive_directory_iterator it(root, opts, ec), end; !ec && it != end; it.increment(ec))
                {
                    const auto& name = it->path().filename().string();
                    if(!name.empty() && name[0] == '.')
                    {
                        if(it->is_directory(ec))
                            it.disable_recursion_pending();
                        continue;
                    }
                    if(it->is_regular_file(ec) && accept(it->path()))
                        out.PushBack(it->path().string());
                }
            }
            else
            {
                for(fs::directory_iterator it(root, opts, ec), end; !ec && it != end; it.increment(ec))
                {
                    const auto& name = it->path().filename().string();
                    if(!name.empty() && name[0] != '.' && it->is_regular_file(ec) && accept(it->path()))
                        out.PushBack(it->path().string());
                }
            }
            if(!out.Empty())
                std::sort(out.Data(), out.Data() + out.Size());
        }

        u32 Queue(const std::string& root, bool recursive, const std::string& extensionsCsv)
        {
            Job* job = new Job();
            {
                std::lock_guard<std::mutex> lock(s_Lock);
                job->id = s_NextId++;
                s_Jobs.PushBack(job);
            }
            job->thread = std::thread([job, root, recursive, extensionsCsv]()
                                      {
                TDArray<std::string> exts;
                ParseExtensions(extensionsCsv, exts);
                List(root, recursive, exts, job->files);
                job->done = true; });
            return job->id;
        }

        u32 QueueMissing(TDArray<std::string>&& paths)
        {
            Job* job = new Job();
            {
                std::lock_guard<std::mutex> lock(s_Lock);
                job->id = s_NextId++;
                s_Jobs.PushBack(job);
            }
            job->thread = std::thread([job, paths = std::move(paths)]()
                                      {
                std::error_code ec;
                for(u32 i = 0; i < paths.Size(); i++)
                    if(!fs::exists(fs::path(paths[i]), ec))
                        job->files.PushBack(paths[i]);
                job->done = true; });
            return job->id;
        }

        bool Poll(u32 id, TDArray<std::string>& out)
        {
            Job* job = nullptr;
            {
                std::lock_guard<std::mutex> lock(s_Lock);
                for(u32 i = 0; i < s_Jobs.Size(); i++)
                {
                    if(s_Jobs[i]->id == id)
                    {
                        if(!s_Jobs[i]->done.load())
                            return false;
                        job = s_Jobs[i];
                        s_Jobs[i] = s_Jobs.Back();
                        s_Jobs.PopBack();
                        break;
                    }
                }
            }
            if(!job)
                return false;
            job->thread.join();
            out = std::move(job->files);
            delete job;
            return true;
        }

        void Shutdown()
        {
            std::lock_guard<std::mutex> lock(s_Lock);
            for(u32 i = 0; i < s_Jobs.Size(); i++)
            {
                if(s_Jobs[i]->thread.joinable())
                    s_Jobs[i]->thread.join();
                delete s_Jobs[i];
            }
            s_Jobs.Clear();
        }
    }
}
