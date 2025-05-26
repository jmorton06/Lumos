#pragma once

#include "Matrix4.h"
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4244) // Conversion from 'double' to 'float'
#pragma warning(disable : 4702) // unreachable code
#endif

#include "Core/Core.h"

#include <cstdlib>
#include <cmath>
#include <limits>
#include <type_traits>
#include "Maths/MathsFwd.h"

namespace Lumos
{
    namespace Maths
    {
        /// Intersection test result.
        enum Intersection : u8
        {
            OUTSIDE    = 0,
            INTERSECTS = 1,
            INSIDE     = 2
        };
#undef M_PI
        static constexpr float M_PI              = 3.14159265358979323846264338327950288f;
        static constexpr float M_2PI             = M_PI * 2.0f;
        static constexpr float M_HALF_PI         = M_PI * 0.5f;
        static constexpr int M_MIN_INT           = 0x80000000;
        static constexpr int M_MAX_INT           = 0x7fffffff;
        static constexpr unsigned M_MIN_UNSIGNED = 0x00000000;
        static constexpr unsigned M_MAX_UNSIGNED = 0xffffffff;

        static constexpr float M_EPSILON       = 0.000001f;
        static constexpr float M_LARGE_EPSILON = 0.00005f;
        static constexpr float M_MIN_NEARCLIP  = 0.01f;
        static constexpr float M_MAX_FOV       = 160.0f;
        static constexpr float M_LARGE_VALUE   = 100000000.0f;
        static constexpr float M_INFINITY      = 1e30f;// std::numeric_limits<float>::infinity();
        static constexpr float M_DEGTORAD      = M_PI / 180.0f;
        static constexpr float M_DEGTORAD_2    = M_PI / 360.0f; // M_DEGTORAD / 2.f
        static constexpr float M_RADTODEG      = 1.0f / M_DEGTORAD;

        template <typename T>
        T Squared(T v)
        {
            return v * v;
        }

        /// Check whether two floating point values are equal within accuracy.
        template <class T>
        inline bool Equals(T lhs, T rhs, T eps = M_EPSILON)
        {
            return lhs + eps >= rhs && lhs - eps <= rhs;
        }

        /// Linear interpolation between two values.
        template <class T, class U>
        inline T Lerp(T lhs, T rhs, U t)
        {
            return lhs * (1.0 - t) + rhs * t;
        }

        /// Inverse linear interpolation between two values.
        template <class T>
        inline T InverseLerp(T lhs, T rhs, T x)
        {
            return (x - lhs) / (rhs - lhs);
        }

        /// Return the smaller of two values.
        template <class T, class U>
        inline T Min(T lhs, U rhs)
        {
            return lhs < rhs ? lhs : rhs;
        }

        /// Return the larger of two values.
        template <class T, class U>
        inline T Max(T lhs, U rhs)
        {
            return lhs > rhs ? lhs : rhs;
        }

        /// Return absolute value of a value
        template <class T>
        inline T Abs(T value)
        {
            return value >= 0.0 ? value : -value;
        }

        /// Return the sign of a float (-1, 0 or 1.)
        template <class T>
        inline T Sign(T value)
        {
            return value > 0.0 ? 1.0 : (value < 0.0 ? -1.0 : 0.0);
        }

        /// Convert degrees to radians.
        template <class T>
        inline T ToRadians(const T degrees)
        {
            return M_DEGTORAD * degrees;
        }

        /// Convert radians to degrees.
        template <class T>
        inline T ToDegrees(const T radians)
        {
            return M_RADTODEG * radians;
        }

        /// Return a representation of the specified floating-point value as a single format bit layout.
        inline unsigned FloatToRawIntBits(float value)
        {
            unsigned u = *((unsigned*)&value);
            return u;
        }

        /// Check whether a floating point value is NaN.
        template <class T>
        inline bool IsNaN(T value)
        {
            return std::isnan(value);
        }

        /// Check whether a floating point value is positive or negative infinity
        template <class T>
        inline bool IsInf(T value)
        {
            return std::isinf(value);
        }

        /// Clamp a number to a range.
        template <class T>
        inline T Clamp(T value, T min, T max)
        {
            if(value < min)
                return min;
            else if(value > max)
                return max;
            else
                return value;
        }

        /// Smoothly damp between values.
        template <class T>
        inline T SmoothStep(T lhs, T rhs, T t)
        {
            t = Clamp((t - lhs) / (rhs - lhs), T(0.0), T(1.0)); // Saturate t
            return t * t * (3.0 - 2.0 * t);
        }

        /// Return sine of an angle in degrees.
        template <class T>
        inline T Sin(T angle)
        {
            return sin(angle * M_DEGTORAD);
        }

        /// Return cosine of an angle in degrees.
        template <class T>
        inline T Cos(T angle)
        {
            return cos(angle * M_DEGTORAD);
        }

        /// Return tangent of an angle in degrees.
        template <class T>
        inline T Tan(T angle)
        {
            return tan(angle * M_DEGTORAD);
        }

        /// Return arc sine in degrees.
        template <class T>
        inline T Asin(T x)
        {
            return M_RADTODEG * asin(Clamp(x, T(-1.0), T(1.0)));
        }

        /// Return arc cosine in degrees.
        template <class T>
        inline T Acos(T x)
        {
            return M_RADTODEG * acos(Clamp(x, T(-1.0), T(1.0)));
        }

        /// Return arc tangent in degrees.
        template <class T>
        inline T Atan(T x)
        {
            return M_RADTODEG * atan(x);
        }

        /// Return arc tangent of y/x in degrees.
        template <class T>
        inline T Atan2(T y, T x)
        {
            return M_RADTODEG * atan2(y, x);
        }

        /// Return X in power Y.
        template <class T>
        inline T Pow(T x, T y)
        {
            return pow(x, y);
        }

        /// Return natural logarithm of X.
        template <class T>
        inline T Ln(T x)
        {
            return log(x);
        }

        /// Return square root of X.
        template <class T>
        inline T Sqrt(T x)
        {
            return sqrt(x);
        }

        /// Return remainder of X/Y.
        template <typename T, typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr>
        inline T Mod(T x, T y)
        {
            return fmod(x, y);
        }

        /// Return remainder of X/Y.
        template <typename T, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
        inline T Mod(T x, T y)
        {
            return x % y;
        }

        /// Return positive remainder of X/Y.
        template <typename T, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
        inline T AbsMod(T x, T y)
        {
            const T result = x % y;
            return result < 0 ? result + y : result;
        }

        /// Return fractional part of passed value in range [0, 1).
        template <class T>
        inline T Fract(T value)
        {
            return value - floor(value);
        }

        /// Round value down.
        template <class T>
        inline T Floor(T x)
        {
            return floor(x);
        }

        /// Round value down. Returns integer value.
        template <class T>
        inline int FloorToInt(T x)
        {
            return static_cast<int>(floor(x));
        }

        /// Round value to nearest integer.
        template <class T>
        inline T Round(T x)
        {
            return round(x);
        }

        /// Round value to nearest integer.
        template <class T>
        inline int RoundToInt(T x)
        {
            return static_cast<int>(round(x));
        }

        /// Round value to nearest multiple.
        template <class T>
        inline T RoundToNearestMultiple(T x, T multiple)
        {
            T mag       = Abs(x);
            multiple    = Abs(multiple);
            T remainder = Mod(mag, multiple);
            if(remainder >= multiple / 2)
                return (FloorToInt<T>(mag / multiple) * multiple + multiple) * Sign(x);
            else
                return (FloorToInt<T>(mag / multiple) * multiple) * Sign(x);
        }

        /// Round value up.
        template <class T>
        inline T Ceil(T x)
        {
            return ceil(x);
        }

        /// Round value up.
        template <class T>
        inline int CeilToInt(T x)
        {
            return static_cast<int>(ceil(x));
        }

        /// Check whether an unsigned integer is a power of two.
        bool IsPowerOfTwo(unsigned value);

        /// Round up to next power of two.
        unsigned NextPowerOfTwo(unsigned value);

        /// Round up or down to the closest power of two.
        unsigned ClosestPowerOfTwo(unsigned value);

        /// Return log base two or the MSB position of the given value.
        unsigned LogBaseTwo(unsigned value);

        /// Count the number of set bits in a mask.
        unsigned CountSetBits(unsigned value);

        /// Update a hash with the given 8-bit value using the SDBM algorithm.
        constexpr unsigned SDBMHash(unsigned hash, unsigned char c);

        /// Convert float to half float
        unsigned short FloatToHalf(float value);

        /// Convert half float to float
        float HalfToFloat(unsigned short value);

        /// Wrap a value fitting it in the range defined by [min, max)
        template <typename T>
        inline T Wrap(T value, T min, T max)
        {
            T range = max - min;
            return min + Mod(value, range);
        }

        /// Calculate both sine and cosine, with angle in degrees.
        void SinCos(float angle, float& sin, float& cos);
        uint32_t nChoosek(uint32_t n, uint32_t k);
        Vec3 ComputeClosestPointOnSegment(const Vec3& segPointA, const Vec3& segPointB, const Vec3& pointC);
        void ClosestPointBetweenTwoSegments(const Vec3& seg1PointA, const Vec3& seg1PointB,
                                            const Vec3& seg2PointA, const Vec3& seg2PointB,
                                            Vec3& closestPointSeg1, Vec3& closestPointSeg2);

        bool AreVectorsParallel(const Vec3& v1, const Vec3& v2);

        Vec2 WorldToScreen(const Vec3& worldPos, const Mat4& mvp, float width, float height, float winPosX = 0.0f, float winPosY = 0.0f);

        void SetScale(Mat4& transform, float scale);
        void SetScale(Mat4& transform, const Vec3& scale);
        void SetRotation(Mat4& transform, const Vec3& rotation);
        void SetTranslation(Mat4& transform, const Vec3& translation);

        Vec3 GetScale(const Mat4& transform);
        Vec3 GetRotation(const Mat4& transform);

        Mat4 Mat4FromTRS(const Vec3& translation, const Vec3& rotation, const Vec3& scale);
        Mat4 ToMat4(const Quat& quat);

        Vec3 Cross(const Vec3& a, const Vec3& b);
        float Dot(const Vec3& a, const Vec3& b);
        float Length(const Vec2& vec);
        float Length(const Vec3& vec);
        float Length2(const Vec3& vec);
        float Distance(const Vec2& a, const Vec2& b);
        float Distance2(const Vec2& a, const Vec2& b);
        float Distance(const Vec3& a, const Vec3& b);
        float Distance2(const Vec3& a, const Vec3& b);
        float Distance(const Vec4& a, const Vec4& b);
        float Distance2(const Vec4& a, const Vec4& b);
        Matrix3 Transpose(const Mat3& mat);
        Matrix4 Transpose(const Mat4& mat);

        float* ValuePtr(Vec2& vec);
        float* ValuePtr(Vec3& vec);
        float* ValuePtr(Vec4& vec);
        float* ValuePtr(Quat& quat);
        float* ValuePtr(Mat3& mat);
        float* ValuePtr(Mat4& mat);

        const float* ValuePtr(const Vec2& vec);
        const float* ValuePtr(const Vec3& vec);
        const float* ValuePtr(const Vec4& vec);
        const float* ValuePtr(const Quat& quat);
        const float* ValuePtr(const Mat3& mat);
        const float* ValuePtr(const Mat4& mat);

        void TestMaths();

        float SineOut(float time);
        float SineIn(float time);
        float SineInOut(float time);

        float ExponentialOut(float time);
        float ExponentialIn(float time);
        float ExponentialInOut(float time);

        float ElasticIn(float time, float period);
        float ElasticOut(float time, float period);
        float ElasticInOut(float time, float period);

        bool AnimateToTarget(float* value, float target, float delta_t, float rate);

        void Print(const Vector3& vec);
        void Print(const Vector4& vec);
        void Print(const Quaternion& vec);
        void Print(const Matrix4& vec);
    }
}
#ifdef _MSC_VER
#pragma warning(pop)
#endif
