#include "Precompiled.h"
#include "SSEUtilities.h"

#ifdef LUMOS_SSE

namespace Lumos
{
    __m128 Mat2Mul(__m128 vec1, __m128 vec2)
    {
        return _mm_add_ps(_mm_mul_ps(vec1, VecSwizzle(vec2, 0, 3, 0, 3)),
                          _mm_mul_ps(VecSwizzle(vec1, 1, 0, 3, 2), VecSwizzle(vec2, 2, 1, 2, 1)));
    }

    // 2x2 row major Matrix adjugate multiply (A#)*B
    __m128 Mat2AdjMul(__m128 vec1, __m128 vec2)
    {
        return _mm_sub_ps(_mm_mul_ps(VecSwizzle(vec1, 3, 3, 0, 0), vec2),
                          _mm_mul_ps(VecSwizzle(vec1, 1, 1, 2, 2), VecSwizzle(vec2, 2, 3, 0, 1)));
    }

    // 2x2 row major Matrix multiply adjugate A*(B#)
    __m128 Mat2MulAdj(__m128 vec1, __m128 vec2)
    {
        return _mm_sub_ps(_mm_mul_ps(vec1, VecSwizzle(vec2, 3, 0, 3, 0)),
                          _mm_mul_ps(VecSwizzle(vec1, 1, 0, 3, 2), VecSwizzle(vec2, 2, 1, 2, 1)));
    }

    float GetValue(const __m128& v, const int index)
    {
#ifdef LUMOS_PLATFORM_WINDOWS
        return v.m128_f32[index];
#else
        return v[index];
#endif
    }
}

#endif