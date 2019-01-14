#pragma once
#include "JM.h"
#define JM_SSE //TODO : remove

#ifdef JM_SSE
#define JM_SSEVEC4
#define JM_SSEVEC3
#define JM_SSEVEC2
#define JM_SSEMAT3
#define JM_SSEMAT4
#define JM_SSEQUAT

#include <smmintrin.h>

inline float GetValue(const __m128& v, const int index)
{
#ifdef JM_PLATFORM_WINDOWS
	return v.m128_f32[index];
#else
	return v[index];
#endif
}
#endif
