#pragma once

#ifdef LUMOS_SSE

#include <smmintrin.h>
namespace Lumos
{
    // https://lxjk.github.io/2017/09/03/Fast-4x4-Matrix-Inverse-with-SSE-SIMD-Explained.html

#define MakeShuffleMask(x, y, z, w) (x | (y << 2) | (z << 4) | (w << 6))

    // vec(0, 1, 2, 3) -> (vec[x], vec[y], vec[z], vec[w])
#define VecSwizzle(vec, x, y, z, w) _mm_shuffle_ps(vec, vec, MakeShuffleMask(x, y, z, w))
#define VecSwizzle1(vec, x) _mm_shuffle_ps(vec, vec, MakeShuffleMask(x, x, x, x))
    // special swizzle
#define VecSwizzle_0101(vec) _mm_movelh_ps(vec, vec)
#define VecSwizzle_2323(vec) _mm_movehl_ps(vec, vec)
#define VecSwizzle_0022(vec) _mm_moveldup_ps(vec)
#define VecSwizzle_1133(vec) _mm_movehdup_ps(vec)

    // return (vec1[x], vec1[y], vec2[z], vec2[w])
#define VecShuffle(vec1, vec2, x, y, z, w) _mm_shuffle_ps(vec1, vec2, MakeShuffleMask(x, y, z, w))
    // special shuffle
#define VecShuffle_0101(vec1, vec2) _mm_movelh_ps(vec1, vec2)
#define VecShuffle_2323(vec1, vec2) _mm_movehl_ps(vec2, vec1)

    // for row major matrix
    // we use __m128 to represent 2x2 matrix as A = | A0  A1 |
    //                                              | A2  A3 |
    // 2x2 row major Matrix multiply A*B
    __m128 Mat2Mul(__m128 vec1, __m128 vec2);
    // 2x2 row major Matrix adjugate multiply (A#)*B
    __m128 Mat2AdjMul(__m128 vec1, __m128 vec2);
    // 2x2 row major Matrix multiply adjugate A*(B#)
    __m128 Mat2MulAdj(__m128 vec1, __m128 vec2);

    float GetValue(const __m128& v, const int index);
}

#endif
