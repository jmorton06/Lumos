#include "Precompiled.h"
#include "JobSystem.h"
#include "Maths/MathsUtilities.h"
#include "Core/DataStructures/TDArray.h"
#include "Core/Thread.h"

#include <atomic>
#include <thread>
#include <condition_variable>
#include <deque>

#ifdef LUMOS_PLATFORM_WINDOWS
#define NOMINMAX
#include <Windows.h>
#include <comdef.h>
#elif LUMOS_PLATFORM_MACOS
#include <pthread.h>
#include <sys/types.h>
#include <mach/mach.h>
#include <mach/thread_policy.h>
#endif

namespace Lumos
{
    namespace System
    {
        class SpinLock
        {
            std::atomic_flag locked = ATOMIC_FLAG_INIT;

        public:
            inline void lock()
            {
                LUMOS_PROFILE_FUNCTION_LOW();
                int spin = 0;
                while(!TryLock())
                {
#if !defined(LUMOS_PLATFORM_MACOS) && !defined(LUMOS_PLATFORM_IOS)
                    if(spin < 10)
                    {
                        _mm_pause(); // SMT thread swap can occur here
                    }
                    else
                    {
                        std::this_thread::yield(); // OS thread swap can occur here. It is important to keep it as fallback, to avoid any chance of lockup by busy wait
                    }
#endif
                    spin++;
                }
            }

            inline bool TryLock()
            {
                return !locked.test_and_set(std::memory_order_acquire);
            }

            inline void unlock()
            {
                locked.clear(std::memory_order_release);
            }
        };

        namespace JobSystem
        {
            struct Job
            {
                Context* ctx;
                std::function<void(JobDispatchArgs)> task;
                uint32_t groupID;
                uint32_t groupJobOffset;
                uint32_t groupJobEnd;
                uint32_t sharedmemory_size;
            };

            struct JobQueue
            {
                std::deque<Job> queue;
                std::mutex locker;

                inline void push_back(const Job& item)
                {
                    std::scoped_lock lock(locker);
                    queue.push_back(item);
                }

                inline bool pop_front(Job& item)
                {
                    std::scoped_lock lock(locker);
                    if(queue.empty())
                    {
                        return false;
                    }
                    item = std::move(queue.front());
                    queue.pop_front();
                    return true;
                }
            };

            // This structure is responsible to stop worker thread loops.
            //    Once this is destroyed, worker threads will be woken up and end their loops.
            struct InternalState
            {
                uint32_t numCores   = 0;
                uint32_t numThreads = 0;
                std::unique_ptr<JobQueue[]> jobQueuePerThread;
                std::atomic_bool alive { true };
                std::condition_variable wakeCondition;
                std::mutex wakeMutex;
                std::atomic<uint32_t> nextQueue { 0 };
                TDArray<std::thread> threads;

                ~InternalState()
                {
                    LUMOS_PROFILE_FUNCTION_LOW();
                    alive.store(false); // indicate that new jobs cannot be started from this point
                    bool wake_loop = true;
                    std::thread waker([&]
                                      {
                        while (wake_loop)
                        {
                            wakeCondition.notify_all(); // wakes up sleeping worker threads
                        } });
                    for(auto& thread : threads)
                    {
                        if(thread.joinable())
                            thread.join();
                    }
                    wake_loop = false;
                    if(waker.joinable())
                        waker.join();
                }
            };
            static InternalState* internal_state = nullptr;

            // Start working on a job queue
            //    After the job queue is finished, it can switch to an other queue and steal jobs from there
            inline void work(uint32_t startingQueue)
            {
                LUMOS_PROFILE_FUNCTION_LOW();
                Job job;
                for(uint32_t i = 0; i < internal_state->numThreads; ++i)
                {
                    JobQueue& job_queue = internal_state->jobQueuePerThread[startingQueue % internal_state->numThreads];
                    while(job_queue.pop_front(job))
                    {
                        JobDispatchArgs args;
                        args.groupID = job.groupID;
                        if(job.sharedmemory_size > 0)
                        {
                            thread_local static TDArray<uint8_t> shared_allocation_data;
                            shared_allocation_data.Reserve(job.sharedmemory_size);
                            args.sharedmemory = shared_allocation_data.Data();
                        }
                        else
                        {
                            args.sharedmemory = nullptr;
                        }

                        for(uint32_t j = job.groupJobOffset; j < job.groupJobEnd; ++j)
                        {
                            args.jobIndex          = j;
                            args.groupIndex        = j - job.groupJobOffset;
                            args.isFirstJobInGroup = (j == job.groupJobOffset);
                            args.isLastJobInGroup  = (j == job.groupJobEnd - 1);
                            job.task(args);
                        }

                        job.ctx->counter.fetch_sub(1);
                    }
                    startingQueue++; // go to next queue
                }
            }

            void OnInit(uint32_t reservedThreads)
            {
                LUMOS_PROFILE_FUNCTION();

                if(!internal_state)
                    internal_state = new InternalState();

                if(internal_state->numThreads > 0)
                    return;

                // Retrieve the number of hardware threads in this System:
                internal_state->numCores = std::thread::hardware_concurrency();

                // Calculate the actual number of worker threads we want:
                internal_state->numThreads = Lumos::Maths::Max(1u, internal_state->numCores - reservedThreads);

                // Keep one for update thread
                internal_state->jobQueuePerThread.reset(new JobQueue[internal_state->numThreads]);
                internal_state->threads.Reserve(internal_state->numThreads);

                for(uint32_t threadID = 0; threadID < internal_state->numThreads; ++threadID)
                {
                    std::thread& worker = internal_state->threads.EmplaceBack([threadID]
                                                                              {
                                ThreadContext& threadContext = *GetThreadContext();
                                threadContext = ThreadContextAlloc();
                                String8 name = PushStr8F(threadContext.ScratchArenas[0], "JobSystem_%u", threadID);
                                LUMOS_PROFILE_SETTHREADNAME((const char*)name.str);
                                SetThreadName(name);

                                while (internal_state->alive.load())
                                {
                                    work(threadID);

                                    // finished with jobs, put to sleep
                                    std::unique_lock<std::mutex> lock(internal_state->wakeMutex);
                                    internal_state->wakeCondition.wait(lock);

                                } });

#ifdef LUMOS_PLATFORM_WINDOWS
                    // Do Windows-specific thread setup:
                    HANDLE handle = (HANDLE)worker.native_handle();

                    // Put each thread on to dedicated core
                    DWORD_PTR affinityMask    = 1ull << threadID;
                    DWORD_PTR affinity_result = SetThreadAffinityMask(handle, affinityMask);
                    LUMOS_ASSERT(affinity_result > 0);

                    // Increase thread priority:
                    // BOOL priority_result = SetThreadPriority(handle, THREAD_PRIORITY_HIGHEST);
                    // LUMOS_ASSERT(priority_result != 0, "");

                    // Name the thread:
                    std::wstring wthreadname = L"JobSystem_" + std::to_wstring(threadID);
                    HRESULT hr               = SetThreadDescription(handle, wthreadname.c_str());

                    LUMOS_ASSERT(SUCCEEDED(hr));

#elif LUMOS_PLATFORM_LINUX

#define handle_error_en(en, msg) \
    do {                         \
        errno = en;              \
        perror(msg);             \
    } while(0)

                    int ret;
                    cpu_set_t cpuset;
                    CPU_ZERO(&cpuset);
                    size_t cpusetsize = sizeof(cpuset);

                    CPU_SET(threadID, &cpuset);
                    ret = pthread_setaffinity_np(worker.native_handle(), cpusetsize, &cpuset);
                    if(ret != 0)
                        handle_error_en(ret, std::string(" pthread_setaffinity_np[" + std::to_string(threadID) + ']').c_str());

                    // Name the thread
                    std::string thread_name = "Job_" + std::to_string(threadID);
                    ret                     = pthread_setname_np(worker.native_handle(), thread_name.c_str());
                    if(ret != 0)
                        handle_error_en(ret, std::string(" pthread_setname_np[" + std::to_string(threadID) + ']').c_str());

#elif LUMOS_PLATFORM_MACOS
                    thread_affinity_policy affinity_tag;
                    affinity_tag.affinity_tag = threadID + 1;
                    auto thread               = worker.native_handle();
                    thread_policy_set(pthread_mach_thread_np(pthread_self()), THREAD_AFFINITY_POLICY, (integer_t*)&affinity_tag, THREAD_AFFINITY_POLICY_COUNT);

                    // pthread_setname_np((const char*)name.str);
#endif

                    worker.detach();
                }

                LUMOS_LOG_INFO("Initialised JobSystem with [{0} cores] [{1} threads]", internal_state->numCores, internal_state->numThreads);
            }

            void Release()
            {
                delete internal_state;
                internal_state = nullptr;
            }

            uint32_t GetThreadCount()
            {
                return internal_state->numThreads;
            }

            void Execute(Context& ctx, const std::function<void(JobDispatchArgs)>& task)
            {
                LUMOS_PROFILE_FUNCTION_LOW();
                // Context state is updated:
                ctx.counter.fetch_add(1);

                Job job;
                job.ctx               = &ctx;
                job.task              = task;
                job.groupID           = 0;
                job.groupJobOffset    = 0;
                job.groupJobEnd       = 1;
                job.sharedmemory_size = 0;

                internal_state->jobQueuePerThread[internal_state->nextQueue.fetch_add(1) % internal_state->numThreads].push_back(job);
                internal_state->wakeCondition.notify_one();
            }

            void Dispatch(Context& ctx, uint32_t jobCount, uint32_t groupSize, const std::function<void(JobDispatchArgs)>& task, size_t sharedmemory_size)
            {
                LUMOS_PROFILE_FUNCTION_LOW();
                if(jobCount == 0 || groupSize == 0)
                {
                    return;
                }

                const uint32_t groupCount = DispatchGroupCount(jobCount, groupSize);

                // Context state is updated:
                ctx.counter.fetch_add(groupCount);

                Job job;
                job.ctx               = &ctx;
                job.task              = task;
                job.sharedmemory_size = (uint32_t)sharedmemory_size;

                for(uint32_t groupID = 0; groupID < groupCount; ++groupID)
                {
                    // For each group, generate one real job:
                    job.groupID        = groupID;
                    job.groupJobOffset = groupID * groupSize;
                    job.groupJobEnd    = std::min(job.groupJobOffset + groupSize, jobCount);

                    internal_state->jobQueuePerThread[internal_state->nextQueue.fetch_add(1) % internal_state->numThreads].push_back(job);
                }

                internal_state->wakeCondition.notify_one();
            }

            uint32_t DispatchGroupCount(uint32_t jobCount, uint32_t groupSize)
            {
                // Calculate the amount of job groups to dispatch (overestimate, or "ceil"):
                return (jobCount + groupSize - 1) / groupSize;
            }

            bool IsBusy(const Context& ctx)
            {
                // Whenever the main thread label is not reached by the workers, it indicates that some worker is still alive
                return ctx.counter.load() > 0;
            }

            void Wait(const Context& ctx)
            {
                LUMOS_PROFILE_FUNCTION_LOW();
                if(IsBusy(ctx))
                {
                    // Wake any threads that might be sleeping:
                    internal_state->wakeCondition.notify_all();

                    // work() will pick up any jobs that are on stand by and execute them on this thread:
                    work(internal_state->nextQueue.fetch_add(1) % internal_state->numThreads);

                    while(IsBusy(ctx))
                    {
                        // If we are here, then there are still remaining jobs that work() couldn't pick up.
                        //    In this case those jobs are not standing by on a queue but currently executing
                        //    on other threads, so they cannot be picked up by this thread.
                        //    Allow to swap out this thread by OS to not spin endlessly for nothing
                        std::this_thread::yield();
                    }
                }
            }
        }
    }
}
