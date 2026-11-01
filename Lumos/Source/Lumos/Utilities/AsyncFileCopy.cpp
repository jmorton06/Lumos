#include "Precompiled.h"
#include "AsyncFileCopy.h"
#include "Maths/MathsUtilities.h"

#include <atomic>
#include <filesystem>
#include <mutex>
#include <thread>

namespace fs = std::filesystem;

namespace Lumos
{
    namespace AsyncFileCopy
    {
        namespace
        {
            struct Job
            {
                u32 id;
                std::string src, dst;
                bool overwrite;
            };

            struct Worker
            {
                std::mutex Lock;
                TDArray<Job> Queue;
                u32 Head = 0;
                TDArray<Result> Done;
                std::thread Thread;
                std::atomic<bool> Running { false };
                std::atomic<u32> Busy { 0 };
                u32 NextId = 1;
            };

            Worker& Get()
            {
                static Worker s_Worker;
                return s_Worker;
            }

            void Run()
            {
                Worker& w = Get();
                while(w.Running.load())
                {
                    Job job { 0, {}, {}, false };
                    {
                        std::lock_guard<std::mutex> lock(w.Lock);
                        if(w.Head < w.Queue.Size())
                        {
                            job = std::move(w.Queue[w.Head++]);
                            if(w.Head == w.Queue.Size())
                            {
                                w.Queue.Clear();
                                w.Head = 0;
                            }
                            w.Busy++;
                        }
                    }
                    if(job.id == 0)
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                        continue;
                    }
                    Result r;
                    r.id = job.id, r.src = job.src, r.dst = job.dst;
                    std::error_code ec;
                    fs::path dst(job.dst);
                    if(dst.has_parent_path())
                        fs::create_directories(dst.parent_path(), ec);
                    ec.clear();
                    auto opts = job.overwrite ? fs::copy_options::overwrite_existing : fs::copy_options::skip_existing;
                    fs::copy_file(fs::path(job.src), dst, opts, ec);
                    if(ec)
                        r.error = ec.message();
                    else
                        r.bytes = (u64)fs::file_size(dst, ec);
                    std::lock_guard<std::mutex> lock(w.Lock);
                    w.Done.PushBack(std::move(r));
                    w.Busy--;
                }
            }
        }

        u32 Queue(const std::string& src, const std::string& dst, bool overwrite)
        {
            Worker& w = Get();
            u32 id;
            {
                std::lock_guard<std::mutex> lock(w.Lock);
                id = w.NextId++;
                w.Queue.PushBack({ id, src, dst, overwrite });
            }
            if(!w.Running.load())
            {
                w.Running = true;
                w.Thread  = std::thread(Run);
            }
            return id;
        }

        u32 Poll(TDArray<Result>& out, u32 max)
        {
            Worker& w = Get();
            std::lock_guard<std::mutex> lock(w.Lock);
            u32 n = (u32)Maths::Min<size_t>(max, w.Done.Size());
            for(u32 i = 0; i < n; i++)
                out.PushBack(std::move(w.Done[i]));
            for(u32 i = n; i < w.Done.Size(); i++)
                w.Done[i - n] = std::move(w.Done[i]);
            w.Done.Resize(w.Done.Size() - n);
            return n;
        }

        u32 Pending()
        {
            Worker& w = Get();
            std::lock_guard<std::mutex> lock(w.Lock);
            return (u32)(w.Queue.Size() - w.Head) + w.Busy.load();
        }

        void Clear()
        {
            Worker& w = Get();
            std::lock_guard<std::mutex> lock(w.Lock);
            w.Queue.Clear();
            w.Head = 0;
        }

        void Shutdown()
        {
            Worker& w = Get();
            if(!w.Running.load())
                return;
            w.Running = false;
            if(w.Thread.joinable())
                w.Thread.join();
        }
    }
}
