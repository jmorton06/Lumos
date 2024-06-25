#include "Precompiled.h"
#include "Utilities/Timer.h"

namespace Lumos
{
    TimeStamp Timer::Now()
    {
        return std::chrono::high_resolution_clock::now();
    }

    double Timer::Duration(TimeStamp start, TimeStamp end, double timeResolution)
    {
        return std::chrono::duration_cast<std::chrono::duration<double>>(end - start).count() * timeResolution;
    }

    float Timer::Duration(TimeStamp start, TimeStamp end, float timeResolution)
    {
        return static_cast<float>(std::chrono::duration_cast<std::chrono::duration<double>>(end - start).count() * timeResolution);
    }
}
