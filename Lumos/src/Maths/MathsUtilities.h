#pragma once
#include "lmpch.h"

namespace Lumos
{
	namespace Maths
	{
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

		static constexpr  float		PI = 3.14159265358979323846f;
		static constexpr  float		PI_OVER_360 = PI / 360.0f;

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
	}
}
