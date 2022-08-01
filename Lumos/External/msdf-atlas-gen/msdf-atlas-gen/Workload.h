
#pragma once

#include <functional>

namespace msdf_atlas {

/**
 * This function allows to split a workload into multiple threads.
 * The worker function:
 *     bool FN(int chunk, int threadNo);
 * should process the given chunk (out of chunks) and return true.
 * If false is returned, the process is interrupted.
 */
class Workload {

public:
    Workload();
    Workload(const std::function<bool(int, int)> &workerFunction, int chunks);
    /// Runs the process and returns true if all chunks have been processed
    bool finish(int threadCount);

private:
    std::function<bool(int, int)> workerFunction;
    int chunks;

    bool finishSequential();
    bool finishParallel(int threadCount);

};

}
