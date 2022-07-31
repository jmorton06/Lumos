
#include "Workload.h"

#include <vector>
#include <thread>
#include <atomic>
#include <algorithm>

namespace msdf_atlas {

Workload::Workload() : chunks(0) { }

Workload::Workload(const std::function<bool(int, int)> &workerFunction, int chunks) : workerFunction(workerFunction), chunks(chunks) { }

bool Workload::finishSequential() {
    for (int i = 0; i < chunks; ++i)
        if (!workerFunction(i, 0))
            return false;
    return true;
}

bool Workload::finishParallel(int threadCount) {
    bool result = true;
    std::atomic<int> next(0);
    std::function<void(int)> threadWorker = [this, &result, &next](int threadNo) {
        for (int i = next++; result && i < chunks; i = next++) {
            if (!workerFunction(i, threadNo))
                result = false;
        }
    };
    std::vector<std::thread> threads;
    threads.reserve(threadCount);
    for (int i = 0; i < threadCount; ++i)
        threads.emplace_back(threadWorker, i);
    for (std::thread &thread : threads)
        thread.join();
    return result;
}

bool Workload::finish(int threadCount) {
    if (!chunks)
        return true;
    if (threadCount == 1 || chunks == 1)
        return finishSequential();
    if (threadCount > 1)
        return finishParallel(std::min(threadCount, chunks));
    return false;
}

}
