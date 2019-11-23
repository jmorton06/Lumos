//#pragma once
//#include "lmpch.h"
//
//namespace Lumos
//{
//	namespace Maths
//	{
//		static const float PI = 3.14159265358979323846264338327950288f;
//		static const float HALF_PI = PI * 0.5f;
//		static const float	PI_OVER_360 = PI / 360.0f;
//
//		static const float DEGTORAD = PI / 180.0f;
//		static const float DEGTORAD_2 = PI / 360.0f;
//		static const float RADTODEG = 1.0f / DEGTORAD;
//
//		static const int MIN_INT = 0x80000000;
//		static const int MAX_INT = 0x7fffffff;
//		static const unsigned MIN_UNSIGNED = 0x00000000;
//		static const unsigned MAX_UNSIGNED = 0xffffffff;
//
//		static const float MIN_NEARCLIP = 0.01f;
//		static const float MAX_FOV = 160.0f;
//		static const float LARGE_VALUE = 100000000.0f;
//		static const float M_INFINITY = (float)HUGE_VAL;
//
//#ifdef PRECISE_MATH_CHECKS
//#define UNIT_EPSILON 0.00001f
//#else
//		//tolerate some more floating point error normally
//#define UNIT_EPSILON 0.001f
//#endif
//
//		_ALWAYS_INLINE_ float Abs(float x) { return x >= 0 ? x : -x; }
//		_ALWAYS_INLINE_ int Abs(int x) { return x >= 0 ? x : -x; }
//		_ALWAYS_INLINE_ double RadiansToDegrees(const double deg) { return deg * 180.0 / PI; };
//		_ALWAYS_INLINE_ float RadiansToDegrees(const float deg) { return deg * 180.0f / PI; };
//		_ALWAYS_INLINE_ double DegreesToRadians(const double rad) { return rad * PI / 180.0; };
//		_ALWAYS_INLINE_ float DegreesToRadians(const float rad) { return rad * PI / 180.0f; };
//		_ALWAYS_INLINE_ float Lerp(const float a, const float b, const float t) { return	a * (1.0f - t) + b * t; };
//
//		template <class T> const T& Max(const T& a, const T& b) { return (a < b) ? b : a; }
//		template <class T> const T& Min(const T& a, const T& b) { return !(b < a) ? a : b; }
//		template <typename T> T Squared(T v) { return v * v; }
//
//		template <class T> inline T Sign(T value) { return value > 0.0 ? 1.0 : (value < 0.0 ? -1.0 : 0.0); }
//
//		/// Convert degrees to radians.
//		template <class T> inline T ToRadians(const T degrees) { return DEGTORAD * degrees; }
//
//		/// Convert radians to degrees.
//		template <class T> inline T ToDegrees(const T radians) { return RADTODEG * radians; }
//
//		template <class T>
//		_ALWAYS_INLINE_ bool Equals(T lhs, T rhs, T eps = UNIT_EPSILON) { return lhs + eps >= rhs && lhs - eps <= rhs; }
//
//		_ALWAYS_INLINE_ bool IsNAN(double p_val) 
//		{
//#ifdef _MSC_VER
//			return _isnan(p_val);
//#elif defined(__GNUC__) && __GNUC__ < 6
//			union {
//				uint64_t u;
//				double f;
//			} ieee754;
//			ieee754.f = p_val;
//			// (unsigned)(0x7ff0000000000001 >> 32) : 0x7ff00000
//			return ((((unsigned)(ieee754.u >> 32) & 0x7fffffff) + ((unsigned)ieee754.u != 0)) > 0x7ff00000);
//#else
//			return isnan(p_val);
//#endif
//		}
//
//		_ALWAYS_INLINE_ bool IsNAN(float p_val)
//		{
//#ifdef _MSC_VER
//			return _isnan(p_val);
//#elif defined(__GNUC__) && __GNUC__ < 6
//			union {
//				uint32_t u;
//				float f;
//			} ieee754;
//			ieee754.f = p_val;
//			// -----------------------------------
//			// (single-precision floating-point)
//			// NaN : s111 1111 1xxx xxxx xxxx xxxx xxxx xxxx
//			//     : (> 0x7f800000)
//			// where,
//			//   s : sign
//			//   x : non-zero number
//			// -----------------------------------
//			return ((ieee754.u & 0x7fffffff) > 0x7f800000);
//#else
//			return isnan(p_val);
//#endif
//		}
//
//		_ALWAYS_INLINE_ bool IsInf(double p_val)
//		{
//#ifdef _MSC_VER
//			return !_finite(p_val);
//#elif defined(__GNUC__) && __GNUC__ < 6
//			union {
//				uint64_t u;
//				double f;
//			} ieee754;
//			ieee754.f = p_val;
//			return ((unsigned)(ieee754.u >> 32) & 0x7fffffff) == 0x7ff00000 &&
//				((unsigned)ieee754.u == 0);
//#else
//			return isinf(p_val);
//#endif
//		}
//
//		_ALWAYS_INLINE_ bool IsInf(float p_val) 
//		{
//#ifdef _MSC_VER
//			return !_finite(p_val);
//#elif defined(__GNUC__) && __GNUC__ < 6
//			union {
//				uint32_t u;
//				float f;
//			} ieee754;
//			ieee754.f = p_val;
//			return (ieee754.u & 0x7fffffff) == 0x7f800000;
//#else
//			return isinf(p_val);
//#endif
//		}
//
//		/// Clamp a number to a range.
//		template <class T> inline T Clamp(T value, T min, T max)
//		{
//			if (value < min)
//				return min;
//			else if (value > max)
//				return max;
//			else
//				return value;
//		}
//
//		/// Smoothly damp between values.
//		template <class T>
//		inline T SmoothStep(T lhs, T rhs, T t)
//		{
//			t = Clamp((t - lhs) / (rhs - lhs), T(0.0), T(1.0)); // Saturate t
//			return t * t * (3.0 - 2.0 * t);
//		}
//
//		/// Return a representation of the specified floating-point value as a single format bit layout.
//		inline unsigned FloatToRawIntBits(float value)
//		{
//			unsigned u = *((unsigned*)&value);
//			return u;
//		}
//
//		/// Return sine of an angle in degrees.
//		template <class T> inline T Sin(T angle) { return sin(angle * DEGTORAD); }
//
//		/// Return cosine of an angle in degrees.
//		template <class T> inline T Cos(T angle) { return cos(angle * DEGTORAD); }
//
//		/// Return tangent of an angle in degrees.
//		template <class T> inline T Tan(T angle) { return tan(angle * DEGTORAD); }
//
//		/// Return arc sine in degrees.
//		template <class T> inline T Asin(T x) { return RADTODEG * asin(Clamp(x, T(-1.0), T(1.0))); }
//
//		/// Return arc cosine in degrees.
//		template <class T> inline T Acos(T x) { return RADTODEG * acos(Clamp(x, T(-1.0), T(1.0))); }
//
//		/// Return arc tangent in degrees.
//		template <class T> inline T Atan(T x) { return RADTODEG * atan(x); }
//
//		/// Return arc tangent of y/x in degrees.
//		template <class T> inline T Atan2(T y, T x) { return RADTODEG * atan2(y, x); }
//
//		/// Return X in power Y.
//		template <class T> inline T Pow(T x, T y) { return pow(x, y); }
//
//		/// Return natural logarithm of X.
//		template <class T> inline T Ln(T x) { return log(x); }
//
//		/// Return square root of X.
//		template <class T> inline T Sqrt(T x) { return sqrt(x); }
//
//		/// Return floating-point remainder of X/Y.
//		template <class T> inline T Mod(T x, T y) { return fmod(x, y); }
//
//		/// Return fractional part of passed value in range [0, 1).
//		template <class T> inline T Fract(T value) { return value - floor(value); }
//
//		/// Round value down.
//		template <class T> inline T Floor(T x) { return floor(x); }
//
//		/// Round value down. Returns integer value.
//		template <class T> inline int FloorToInt(T x) { return static_cast<int>(floor(x)); }
//
//		/// Round value to nearest integer.
//		template <class T> inline T Round(T x) { return round(x); }
//
//		/// Compute average value of the range.
//		template <class Iterator> inline auto Average(Iterator begin, Iterator end) -> typename std::decay<decltype(*begin)>::type
//		{
//			using T = typename std::decay<decltype(*begin)>::type;
//
//			T average{};
//			unsigned size{};
//			for (Iterator it = begin; it != end; ++it)
//			{
//				average += *it;
//				++size;
//			}
//
//			return size != 0 ? average / size : average;
//		}
//
//		/// Round value to nearest integer.
//		template <class T> inline int RoundToInt(T x) { return static_cast<int>(round(x)); }
//
//		/// Round value to nearest multiple.
//		template <class T> inline T RoundToNearestMultiple(T x, T multiple)
//		{
//			T mag = Abs(x);
//			multiple = Abs(multiple);
//			T remainder = Mod(mag, multiple);
//			if (remainder >= multiple / 2)
//				return (FloorToInt<T>(mag / multiple) * multiple + multiple)*Sign(x);
//			else
//				return (FloorToInt<T>(mag / multiple) * multiple)*Sign(x);
//		}
//
//		/// Round value up.
//		template <class T> inline T Ceil(T x) { return ceil(x); }
//
//		/// Round value up.
//		template <class T> inline int CeilToInt(T x) { return static_cast<int>(ceil(x)); }
//
//		/// Check whether an unsigned integer is a power of two.
//		inline bool IsPowerOfTwo(unsigned value)
//		{
//			return !(value & (value - 1));
//		}
//
//		/// Round up to next power of two.
//		inline unsigned NextPowerOfTwo(unsigned value)
//		{
//			// http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
//			--value;
//			value |= value >> 1u;
//			value |= value >> 2u;
//			value |= value >> 4u;
//			value |= value >> 8u;
//			value |= value >> 16u;
//			return ++value;
//		}
//
//		/// Return log base two or the MSB position of the given value.
//		inline unsigned LogBaseTwo(unsigned value)
//		{
//			// http://graphics.stanford.edu/~seander/bithacks.html#IntegerLogObvious
//			unsigned ret = 0;
//			while (value >>= 1)     // Unroll for more speed...
//				++ret;
//			return ret;
//		}
//
//		/// Count the number of set bits in a mask.
//		inline unsigned CountSetBits(unsigned value)
//		{
//			// Brian Kernighan's method
//			unsigned count = 0;
//			for (count = 0; value; count++)
//				value &= value - 1;
//			return count;
//		}
//
//		/// Update a hash with the given 8-bit value using the SDBM algorithm.
//		inline unsigned SDBMHash(unsigned hash, unsigned char c) { return c + (hash << 6u) + (hash << 16u) - hash; }
//	}
//}
