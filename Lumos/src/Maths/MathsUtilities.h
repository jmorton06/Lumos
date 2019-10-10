#pragma once
#include "lmpch.h"

namespace Lumos
{
	namespace Maths
	{
		static constexpr  float		PI = 3.14159265358979323846f;
		static constexpr  float		PI_OVER_360 = PI / 360.0f;

		static constexpr float DEGTORAD = PI / 180.0f;
		static constexpr float DEGTORAD_2 = PI / 360.0f;
		static constexpr float RADTODEG = 1.0f / DEGTORAD;

		template <class T> const T& Max(const T& a, const T& b)
		{
			return (a < b) ? b : a;
		}

		template <class T> const T& Min(const T& a, const T& b)
		{
			return !(b < a) ? a : b;
		}

		template <typename T> T Clamp(const T& value, const T& low, const T& high)
		{
			return value < low ? low : (value > high ? high : value);
		}

		template <typename T> T Squared(T v)
		{
			return v * v;
		}

		inline float Abs(float x)
		{
			return x >= 0 ? x : -x;
		}

		inline int Abs(int x)
		{
			return x >= 0 ? x : -x;
		}

		//Radians to degrees
		static constexpr  double RadiansToDegrees(const double deg)
		{
			return deg * 180.0 / PI;
		};

		static constexpr  float RadiansToDegrees(const float deg)
		{
			return deg * 180.0f / PI;
		};

		//Degrees to radians
		static constexpr double DegreesToRadians(const double rad)
		{
			return rad * PI / 180.0;
		};

		static constexpr  float DegreesToRadians(const float rad)
		{
			return rad * PI / 180.0f;
		};

		static constexpr  float Lerp(const float a, const float b, const float t)
		{
			return	a * (1.0f - t) + b * t;
		};

		static _ALWAYS_INLINE_ bool IsNAN(double p_val) 
		{
#ifdef _MSC_VER
			return _isnan(p_val);
#elif defined(__GNUC__) && __GNUC__ < 6
			union {
				uint64_t u;
				double f;
			} ieee754;
			ieee754.f = p_val;
			// (unsigned)(0x7ff0000000000001 >> 32) : 0x7ff00000
			return ((((unsigned)(ieee754.u >> 32) & 0x7fffffff) + ((unsigned)ieee754.u != 0)) > 0x7ff00000);
#else
			return isnan(p_val);
#endif
		}

		static _ALWAYS_INLINE_ bool IsNAN(float p_val)
		{
#ifdef _MSC_VER
			return _isnan(p_val);
#elif defined(__GNUC__) && __GNUC__ < 6
			union {
				uint32_t u;
				float f;
			} ieee754;
			ieee754.f = p_val;
			// -----------------------------------
			// (single-precision floating-point)
			// NaN : s111 1111 1xxx xxxx xxxx xxxx xxxx xxxx
			//     : (> 0x7f800000)
			// where,
			//   s : sign
			//   x : non-zero number
			// -----------------------------------
			return ((ieee754.u & 0x7fffffff) > 0x7f800000);
#else
			return isnan(p_val);
#endif
		}

		static _ALWAYS_INLINE_ bool IsInf(double p_val)
		{
#ifdef _MSC_VER
			return !_finite(p_val);
#elif defined(__GNUC__) && __GNUC__ < 6
			union {
				uint64_t u;
				double f;
			} ieee754;
			ieee754.f = p_val;
			return ((unsigned)(ieee754.u >> 32) & 0x7fffffff) == 0x7ff00000 &&
				((unsigned)ieee754.u == 0);
#else
			return isinf(p_val);
#endif
		}

		static _ALWAYS_INLINE_ bool IsInf(float p_val) 
		{
#ifdef _MSC_VER
			return !_finite(p_val);
#elif defined(__GNUC__) && __GNUC__ < 6
			union {
				uint32_t u;
				float f;
			} ieee754;
			ieee754.f = p_val;
			return (ieee754.u & 0x7fffffff) == 0x7f800000;
#else
			return isinf(p_val);
#endif
		}
	}
}
