#include "Precompiled.h"
#include "JobSystem.h"
#include "Maths/MathsUtilities.h"
#include "Core/DataStructures/TDArray.h"
#include "Core/DataStructures/TSQueue.h"
#include "Core/Thread.h"
#include "Core/Mutex.h"

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
                    if(spin < 10)
                    {
#if defined(LUMOS_PLATFORM_MACOS) || defined(LUMOS_PLATFORM_IOS)
                        sched_yield();
#elif defined(LUMOS_PLATFORM_WINDOWS)
                        _mm_pause(); // SMT thread swap can occur here
#else
                        _mm_pause();
#endif
                    }
                    else
                    {
#ifdef LUMOS_PLATFORM_WINDOWS
                        SwitchToThread();
#else
                        sched_yield();
#endif // OS thread swap can occur here. It is important to keep it as fallback, to avoid any chance of lockup by busy wait
                    }
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
                Function<void(JobDispatchArgs)> task;
                uint32_t groupID;
                uint32_t groupJobOffset;
                uint32_t groupJobEnd;
                uint32_t sharedmemory_size;
            };

            // Cache-line aligned so adjacent worker queues don't false-share
            // their internal state. TSQueue is already thread-safe so the
            // earlier extra mutex wrapper here was redundant — push/pop now
            // take exactly one lock per op.
            struct alignas(64) JobQueue
            {
                TSQueue<Job> queue;

                inline void push_back(const Job& item)
                {
                    queue.PushBack(item);
                }

                inline bool pop_front(Job& item)
                {
                    return queue.PopFront(item);
                }
            };

            // This structure is responsible to stop worker thread loops.
            //    Once this is destroyed, worker threads will be woken up and end their loops.
            struct InternalState
            {
                uint32_t numCores   = 0;
                uint32_t numThreads = 0;
                JobQueue* jobQueuePerThread;
                std::atomic_bool alive { true };
                ConditionVar* wakeCondition;
                Mutex* wakeMutex;
                std::atomic<uint32_t> nextQueue { 0 };
                // Global count of pushed-but-not-yet-popped jobs across all
                // worker queues. Used as the predicate for ConditionWait so
                // workers never sleep through a notify that arrived between
                // their last pop and entering the wait.
                std::atomic<uint32_t> pendingJobs { 0 };
                TDArray<std::thread> threads;

                InternalState()
                {
                    wakeMutex = new Mutex();
                    MutexInit(wakeMutex);

                    wakeCondition = new ConditionVar();
                    ConditionInit(wakeCondition);
                }

                ~InternalState()
                {
                    LUMOS_PROFILE_FUNCTION_LOW();

                    // Set alive=false WHILE holding wakeMutex so any worker
                    // that's between its predicate check and ConditionWait
                    // sees the new value before sleeping. A single NotifyAll
                    // is then sufficient — no busy-waker required.
                    {
                        ScopedMutex lock(wakeMutex);
                        alive.store(false);
                    }
                    ConditionNotifyAll(wakeCondition);

                    for(auto& thread : threads)
                    {
                        if(thread.joinable())
                            thread.join();
                    }

                    delete[] jobQueuePerThread;
                    ConditionDestroy(wakeCondition);
                    delete wakeCondition;

                    MutexDestroy(wakeMutex);
                    delete wakeMutex;
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
                        internal_state->pendingJobs.fetch_sub(1, std::memory_order_relaxed);
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
                internal_state->jobQueuePerThread = new JobQueue[internal_state->numThreads];
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

                                    // Predicate-protected sleep — only block when
                                    // there's genuinely nothing pending. Producers
                                    // increment pendingJobs under wakeMutex (see
                                    // Execute/Dispatch) so this check + ConditionWait
                                    // cannot race a notify that arrived between the
                                    // last pop and entering wait.
                                    MutexLock(internal_state->wakeMutex);
                                    while(internal_state->alive.load() &&
                                          internal_state->pendingJobs.load(std::memory_order_acquire) == 0)
                                    {
                                        ConditionWait(internal_state->wakeCondition, internal_state->wakeMutex);
                                    }
                                    MutexUnlock(internal_state->wakeMutex);
                                } });

#ifdef LUMOS_PLATFORM_WINDOWS
                    // Do Windows-specific thread setup:
                    HANDLE handle = (HANDLE)worker.native_handle();

                    // Put each thread on to dedicated core. Single-group
                    // affinity mask only spans 64 logical processors; on
                    // bigger hosts SetThreadGroupAffinity would be needed
                    // (left to OS scheduler for now beyond core 64 rather
                    // than invoking UB via 1ull << 64).
                    if(threadID < 64)
                    {
                        DWORD_PTR affinityMask    = 1ull << threadID;
                        DWORD_PTR affinity_result = SetThreadAffinityMask(handle, affinityMask);
                        ASSERT(affinity_result > 0);
                    }

                    // Increase thread priority:
                    // BOOL priority_result = SetThreadPriority(handle, THREAD_PRIORITY_HIGHEST);
                    // ASSERT(priority_result != 0, "");

                    // Name the thread:
                    std::wstring wthreadname = L"JobSystem_" + std::to_wstring(threadID);
#if defined _MSC_VER
                    HRESULT hr = SetThreadDescription(handle, wthreadname.c_str());
                    ASSERT(SUCCEEDED(hr));
#endif

#elif LUMOS_PLATFORM_LINUX

#define handle_error_en(en, msg) \
    do                           \
    {                            \
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
                    thread_policy_set(pthread_mach_thread_np(thread), THREAD_AFFINITY_POLICY, (integer_t*)&affinity_tag, THREAD_AFFINITY_POLICY_COUNT);
#endif
                }

                LINFO("Initialised JobSystem with [%i cores] [%i threads]", internal_state->numCores, internal_state->numThreads);
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

            void Execute(Context& ctx, const Function<void(JobDispatchArgs)>& task)
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

                // Publish the new pending count under wakeMutex so a worker
                // checking the predicate can't miss it before sleeping.
                {
                    ScopedMutex lock(internal_state->wakeMutex);
                    internal_state->pendingJobs.fetch_add(1, std::memory_order_release);
                }
                ConditionNotifyOne(internal_state->wakeCondition);
            }

            void Dispatch(Context& ctx, uint32_t jobCount, uint32_t groupSize, const Function<void(JobDispatchArgs)>& task, size_t sharedmemory_size)
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

                // Publish the new pending count under wakeMutex so workers
                // checking the predicate can't miss it before sleeping.
                {
                    ScopedMutex lock(internal_state->wakeMutex);
                    internal_state->pendingJobs.fetch_add(groupCount, std::memory_order_release);
                }
                // NotifyAll for batches so the work goes wide instead of one
                // worker serialising the whole dispatch.
                if(groupCount > 1)
                    ConditionNotifyAll(internal_state->wakeCondition);
                else
                    ConditionNotifyOne(internal_state->wakeCondition);
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
                    ConditionNotifyAll(internal_state->wakeCondition);

                    // work() will pick up any jobs that are on stand by and execute them on this thread:
                    work(internal_state->nextQueue.fetch_add(1) % internal_state->numThreads);

                    while(IsBusy(ctx))
                    {
                        // If we are here, then there are still remaining jobs that work() couldn't pick up.
                        //    In this case those jobs are not standing by on a queue but currently executing
                        //    on other threads, so they cannot be picked up by this thread.
                        //    Allow to swap out this thread by OS to not spin endlessly for nothing
#ifdef LUMOS_PLATFORM_WINDOWS
                        SwitchToThread();
#else
                        sched_yield();
#endif
                    }
                }
            }
        }
    }
}
