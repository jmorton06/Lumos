#pragma once
#include "LM.h"
//#define LUMOS_SSE //TODO : remove

#ifdef LUMOS_SSE
#define LUMOS_SSEVEC4
#define LUMOS_SSEVEC3
#define LUMOS_SSEVEC2
#define LUMOS_SSEMAT3
#define LUMOS_SSEMAT4
#define LUMOS_SSEQUAT

#include <smmintrin.h>

inline float GetValue(const __m128& v, const int index)
{
#ifdef LUMOS_PLATFORM_WINDOWS
	return v.m128_f32[index];
#else
	return v[index];
#endif
}
#endif

