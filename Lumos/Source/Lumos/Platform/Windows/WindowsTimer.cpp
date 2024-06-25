#include "Precompiled.h"
#include "Utilities/Timer.h"

namespace Lumos
{
    static double GetQueryPerformancePeriod()
    {
        LARGE_INTEGER result;
        QueryPerformanceFrequency(&result);
        return 1.0 / static_cast<double>(result.QuadPart);
    }
    const double freq = GetQueryPerformancePeriod();

    TimeStamp Timer::Now()
    {
        TimeStamp temp;
        QueryPerformanceCounter(&temp);
        return temp;
    }

    double Timer::Duration(TimeStamp start, TimeStamp end, double timeResolution)
    {
        return (end.QuadPart - start.QuadPart) * timeResolution * freq;
    }

    float Timer::Duration(TimeStamp start, TimeStamp end, float timeResolution)
    {
        return static_cast<float>((end.QuadPart - start.QuadPart) * timeResolution * freq);
    }
}
