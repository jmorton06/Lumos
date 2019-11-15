#include "lmpch.h"
#include "JobSystem.h"
#include "Maths/Maths.h"

#include <atomic>
#include <thread>
#include <condition_variable>
#include <deque>

#ifdef LUMOS_PLATFORM_WINDOWS
#define NOMINMAX
#include <Windows.h>
#endif
namespace Lumos 
{
    namespace System 
    {
        // Fixed size very simple thread safe ring buffer
        template <typename T, size_t capacity>
        class ThreadSafeRingBuffer
        {
        public:
            // Push an item to the end if there is free space
            //	Returns true if succesful
            //	Returns false if there is not enough space
            inline bool push_back(const T& item)
            {
                bool result = false;
                lock.lock();
                size_t next = (head + 1) % capacity;
                if (next != tail)
                {
                    data[head] = item;
                    head = next;
                    result = true;
                }
                lock.unlock();
                return result;
            }

            // Get an item if there are any
            //	Returns true if succesful
            //	Returns false if there are no items
            inline bool pop_front(T& item)
            {
                bool result = false;
                lock.lock();
                if (tail != head)
                {
                    item = data[tail];
                    tail = (tail + 1) % capacity;
                    result = true;
                }
                lock.unlock();
                return result;
            }

        private:
            T data[capacity];
            size_t head = 0;
            size_t tail = 0;
            std::mutex lock; // this just works better than a spinlock here (on windows)
        };

        namespace JobSystem
        {
            uint32_t numThreads = 0;
            ThreadSafeRingBuffer<std::function<void()>, 256> jobPool;
            std::condition_variable wakeCondition;
            std::mutex wakeMutex;
            uint64_t currentLabel = 0;
            std::atomic<uint64_t> finishedLabel;

            void OnInit()
            {
                finishedLabel.store(0);

                // Retrieve the number of hardware threads in this System:
                auto numCores = std::thread::hardware_concurrency();

                // Calculate the actual number of worker threads we want:
                numThreads = Lumos::Maths::Max(1U, numCores);

                for (uint32_t threadID = 0; threadID < numThreads; ++threadID)
                {
                    std::thread worker([] {

                        std::function<void()> job;

                        while (true)
                        {
                            if (jobPool.pop_front(job))
                            {
                                job(); // execute job
                                finishedLabel.fetch_add(1); // update worker label state
                            }
                            else
                            {
                                // no job, put thread to sleep
                                std::unique_lock<std::mutex> lock(wakeMutex);
                                wakeCondition.wait(lock);
                            }
                        }

                    });

        #ifdef LUMOS_PLATFORM_WINDOWS
                    // Do Windows-specific thread setup:
                    HANDLE handle = (HANDLE)worker.native_handle();

                    // Put each thread on to dedicated core
                    DWORD_PTR affinityMask = 1ull << threadID; 
                    DWORD_PTR affinity_result = SetThreadAffinityMask(handle, affinityMask);
                    LUMOS_ASSERT(affinity_result > 0,"");
                    // Name the thread:
                    std::wstringstream wss;
                    wss << "JobSystem_" << threadID;
                    HRESULT hr = SetThreadDescription(handle, wss.str().c_str());
                    LUMOS_ASSERT(SUCCEEDED(hr),"");
        #endif // LUMOS_PLATFORM_WINDOWS

                    worker.detach();
                }

                LUMOS_LOG_INFO("Initialised JobSystem with [{0} cores] [{1} threads]" ,numCores, numThreads);
            }

            // This little function will not let the System to be deadlocked while the main thread is waiting for something
            inline void poll()
            {
                wakeCondition.notify_one(); // wake one worker thread
                std::this_thread::yield(); // allow this thread to be rescheduled
            }

            uint32_t GetThreadCount()
            {
                return numThreads;
            }

            void Execute(const std::function<void()>& job)
            {
                // The main thread label state is updated:
                currentLabel += 1;

                // Try to push a new job until it is pushed successfully:
                while (!jobPool.push_back(job)) { poll(); }

                wakeCondition.notify_one(); // wake one thread
            }

            void Dispatch(uint32_t jobCount, uint32_t groupSize, const std::function<void(JobDispatchArgs)>& job)
            {
                if (jobCount == 0 || groupSize == 0)
                {
                    return;
                }

                // Calculate the amount of job groups to dispatch (overestimate, or "ceil"):
                const uint32_t groupCount = (jobCount + groupSize - 1) / groupSize;

                // The main thread label state is updated:
                currentLabel += groupCount;

                for (uint32_t groupIndex = 0; groupIndex < groupCount; ++groupIndex)
                {
                    // For each group, generate one real job:
                    const auto& jobGroup = [jobCount, groupSize, job, groupIndex]() {

                        // Calculate the current group's offset into the jobs:
                        const uint32_t groupJobOffset = groupIndex * groupSize;
                        const uint32_t groupJobEnd = Lumos::Maths::Min(groupJobOffset + groupSize, jobCount);

                        JobDispatchArgs args;
                        args.groupIndex = groupIndex;

                        // Inside the group, loop through all job indices and execute job for each index:
                        for (uint32_t i = groupJobOffset; i < groupJobEnd; ++i)
                        {
                            args.jobIndex = i;
                            job(args);
                        }
                    };

                    // Try to push a new job until it is pushed successfully:
                    while (!jobPool.push_back(jobGroup)) { poll(); }

                    wakeCondition.notify_one(); // wake one thread
                }


            }

            bool IsBusy()
            {
                // Whenever the main thread label is not reached by the workers, it indicates that some worker is still alive
                return finishedLabel.load() < currentLabel;
            }

            void Wait()
            {
                while (IsBusy()) { poll(); }
            }
        }
    }
}
