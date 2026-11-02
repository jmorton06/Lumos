#include "Precompiled.h"
#include "AudioTagReader.h"
#include "Maths/MathsUtilities.h"

#include <atomic>
#include <mutex>
#include <thread>

namespace Lumos
{
    namespace AudioTagReader
    {
        namespace
        {
            struct Job
            {
                u32 id;
                std::string path;
            };

            struct Pool
            {
                std::mutex Lock;
                TDArray<Job> Queue;
                TDArray<Result> Done;
                std::thread Threads[2];
                std::atomic<bool> Running { false };
                std::atomic<u32> Busy { 0 };
                u32 NextId = 1;
                u32 Head   = 0; // FIFO: scans want files in order
            };

            Pool& Get()
            {
                static Pool s_Pool;
                return s_Pool;
            }

            void Worker()
            {
                Pool& p = Get();
                while(p.Running.load())
                {
                    Job job { 0, {} };
                    {
                        std::lock_guard<std::mutex> lock(p.Lock);
                        if(p.Head < p.Queue.Size())
                        {
                            job = std::move(p.Queue[p.Head++]);
                            if(p.Head == p.Queue.Size())
                            {
                                p.Queue.Clear();
                                p.Head = 0;
                            }
                            p.Busy++;
                        }
                    }
                    if(job.id == 0)
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(5));
                        continue;
                    }
                    Result r;
                    r.id   = job.id;
                    r.path = job.path;
                    r.ok   = ReadAudioTags(job.path, r.tags, false);
                    std::lock_guard<std::mutex> lock(p.Lock);
                    p.Done.PushBack(std::move(r));
                    p.Busy--;
                }
            }
        }

        u32 Queue(const std::string& path)
        {
            Pool& p = Get();
            u32 id;
            {
                std::lock_guard<std::mutex> lock(p.Lock);
                id = p.NextId++;
                p.Queue.PushBack({ id, path });
            }
            if(!p.Running.load())
            {
                p.Running = true;
                for(auto& t : p.Threads)
                    t = std::thread(Worker);
            }
            return id;
        }

        u32 Poll(TDArray<Result>& out, u32 max)
        {
            Pool& p = Get();
            std::lock_guard<std::mutex> lock(p.Lock);
            u32 n = (u32)Maths::Min<size_t>(max, p.Done.Size());
            for(u32 i = 0; i < n; i++)
                out.PushBack(std::move(p.Done[i]));
            for(u32 i = n; i < p.Done.Size(); i++)
                p.Done[i - n] = std::move(p.Done[i]);
            p.Done.Resize(p.Done.Size() - n);
            return n;
        }

        u32 Pending()
        {
            Pool& p = Get();
            std::lock_guard<std::mutex> lock(p.Lock);
            return (u32)(p.Queue.Size() - p.Head + p.Done.Size()) + p.Busy.load();
        }

        void Clear()
        {
            Pool& p = Get();
            std::lock_guard<std::mutex> lock(p.Lock);
            p.Queue.Clear();
            p.Head = 0;
            p.Done.Clear();
        }

        void Shutdown()
        {
            Pool& p = Get();
            if(!p.Running.load())
                return;
            p.Running = false;
            for(auto& t : p.Threads)
                if(t.joinable())
                    t.join();
            Clear();
        }
    }
}
