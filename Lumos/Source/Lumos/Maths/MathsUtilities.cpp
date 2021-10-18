#include "Precompiled.h"
#include "Maths/MathsUtilities.h"

namespace Lumos::Maths
{
    void SinCos(float angle, float& sin, float& cos)
    {
        float angleRadians = angle * M_DEGTORAD;
#if defined(HAVE_SINCOSF)
        sincosf(angleRadians, &sin, &cos);
#elif defined(HAVE___SINCOSF)
        __sincosf(angleRadians, &sin, &cos);
#else
        sin = sinf(angleRadians);
        cos = cosf(angleRadians);
#endif
    }

    uint32_t nChoosek(uint32_t n, uint32_t k)
    {
        if(k > n)
            return 0;
        if(k * 2 > n)
            k = n - k;
        if(k == 0)
            return 1;

        uint32_t result = n;
        for(int i = 2; i <= k; ++i)
        {
            result *= (n - i + 1);
            result /= i;
        }
        return result;
    }
}
