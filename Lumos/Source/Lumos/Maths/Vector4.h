#pragma once
#include "Maths/Vector3.h"
#include <iostream>

namespace Lumos::Maths
{

    /// Four-dimensional vector with integer values.
    class IntVector4
    {
    public:
        /// Construct a zero vector.
        IntVector4() noexcept
            : x(0)
            , y(0)
            , z(0)
            , w(0)
        {
        }

        /// Construct from coordinates.
        IntVector4(int x, int y, int z, int w) noexcept
            : x(x)
            , y(y)
            , z(z)
            , w(w)
        {
        }

        /// Construct from an int array.
        explicit IntVector4(const int* data) noexcept
            : x(data[0])
            , y(data[1])
            , z(data[2])
            , w(data[3])
        {
        }

        /// Copy-construct from another vector.
        IntVector4(const IntVector4& rhs) noexcept = default;

        /// Assign from another vector.
        IntVector4& operator=(const IntVector4& rhs) noexcept = default;

        /// Test for equality with another vector.
        bool operator==(const IntVector4& rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w; }

        /// Test for inequality with another vector.
        bool operator!=(const IntVector4& rhs) const { return x != rhs.x || y != rhs.y || z != rhs.z || w != rhs.w; }

        /// Add a vector.
        IntVector4 operator+(const IntVector4& rhs) const { return IntVector4(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w); }

        /// Return negation.
        IntVector4 operator-() const { return IntVector4(-x, -y, -z, -w); }

        /// Subtract a vector.
        IntVector4 operator-(const IntVector4& rhs) const { return IntVector4(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w); }

        /// Multiply with a scalar.
        IntVector4 operator*(int rhs) const { return IntVector4(x * rhs, y * rhs, z * rhs, w * rhs); }

        /// Multiply with a vector.
        IntVector4 operator*(const IntVector4& rhs) const { return IntVector4(x * rhs.x, y * rhs.y, z * rhs.z, w * rhs.w); }

        /// Divide by a scalar.
        IntVector4 operator/(int rhs) const { return IntVector4(x / rhs, y / rhs, z / rhs, w / rhs); }

        /// Divide by a vector.
        IntVector4 operator/(const IntVector4& rhs) const { return IntVector4(x / rhs.x, y / rhs.y, z / rhs.z, w / rhs.w); }

        /// Add-assign a vector.
        IntVector4& operator+=(const IntVector4& rhs)
        {
            x += rhs.x;
            y += rhs.y;
            z += rhs.z;
            w += rhs.w;
            return *this;
        }

        /// Subtract-assign a vector.
        IntVector4& operator-=(const IntVector4& rhs)
        {
            x -= rhs.x;
            y -= rhs.y;
            z -= rhs.z;
            w -= rhs.w;
            return *this;
        }

        /// Multiply-assign a scalar.
        IntVector4& operator*=(int rhs)
        {
            x *= rhs;
            y *= rhs;
            z *= rhs;
            return *this;
        }

        /// Multiply-assign a vector.
        IntVector4& operator*=(const IntVector4& rhs)
        {
            x *= rhs.x;
            y *= rhs.y;
            z *= rhs.z;
            w *= rhs.w;
            return *this;
        }

        /// Divide-assign a scalar.
        IntVector4& operator/=(int rhs)
        {
            x /= rhs;
            y /= rhs;
            z /= rhs;
            w /= rhs;
            return *this;
        }

        /// Divide-assign a vector.
        IntVector4& operator/=(const IntVector4& rhs)
        {
            x /= rhs.x;
            y /= rhs.y;
            z /= rhs.z;
            w /= rhs.w;
            return *this;
        }

        /// Return integer data.
        const int* Data() const { return &x; }

        /// Return as string.

        /// Return hash value for HashSet & HashMap.
        unsigned ToHash() const { return (unsigned)x * 31 * 31 * 31 + (unsigned)y * 31 * 31 + (unsigned)z * 31 + (unsigned)w; }

        /// Return length.
        float Length() const { return sqrtf((float)(x * x + y * y + z * z + w * w)); }

        /// X coordinate.
        int x;
        /// Y coordinate.
        int y;
        /// Z coordinate.
        int z;
        /// W coordinate.
        int w;
    };
    /// Four-dimensional vector.
    class Vector4
    {
    public:
        /// Construct a zero vector.
        Vector4() noexcept
            : x(0.0f)
            , y(0.0f)
            , z(0.0f)
            , w(0.0f)
        {
        }

        explicit Vector4(float x) noexcept
            : x(x)
            , y(x)
            , z(x)
            , w(x)
        {
        }

        Vector4(float x, float y, float z, float w) noexcept
            : x(x)
            , y(y)
            , z(z)
            , w(w)
        {
        }

        /// Copy-construct from another vector.
        Vector4(const Vector4& vector) noexcept = default;

        /// Construct from a 3-dimensional vector and the W coordinate.
        Vector4(const Vector3& vector, float w) noexcept
            : x(vector.x)
            , y(vector.y)
            , z(vector.z)
            , w(w)
        {
        }

        Vector4(const Vector2& vector, float z, float w) noexcept
            : x(vector.x)
            , y(vector.y)
            , z(z)
            , w(w)
        {
        }

        Vector4(float x, const Vector2& vector, float w) noexcept
            : x(x)
            , y(vector.x)
            , z(vector.y)
            , w(w)
        {
        }

        Vector4(float x, float y, const Vector2& vector) noexcept
            : x(x)
            , y(y)
            , z(vector.x)
            , w(vector.y)
        {
        }

        Vector4(const Vector3& vector) noexcept
            : x(vector.x)
            , y(vector.y)
            , z(vector.z)
            , w(1.0f)
        {
        }

        /// Construct from a float array.
        explicit Vector4(const float* data) noexcept
            : x(data[0])
            , y(data[1])
            , z(data[2])
            , w(data[3])
        {
        }

        /// Assign from another vector.
        Vector4& operator=(const Vector4& rhs) noexcept = default;

        /// Test for equality with another vector without epsilon.
        bool operator==(const Vector4& rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w; }

        /// Test for inequality with another vector without epsilon.
        bool operator!=(const Vector4& rhs) const { return x != rhs.x || y != rhs.y || z != rhs.z || w != rhs.w; }

        /// Add a vector.
        Vector4 operator+(const Vector4& rhs) const { return Vector4(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w); }

        /// Return negation.
        Vector4 operator-() const { return Vector4(-x, -y, -z, -w); }

        /// Subtract a vector.
        Vector4 operator-(const Vector4& rhs) const { return Vector4(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w); }

        /// Multiply with a scalar.
        Vector4 operator*(float rhs) const { return Vector4(x * rhs, y * rhs, z * rhs, w * rhs); }

        /// Multiply with a vector.
        Vector4 operator*(const Vector4& rhs) const { return Vector4(x * rhs.x, y * rhs.y, z * rhs.z, w * rhs.w); }

        /// Divide by a scalar.
        Vector4 operator/(float rhs) const { return Vector4(x / rhs, y / rhs, z / rhs, w / rhs); }

        /// Divide by a vector.
        Vector4 operator/(const Vector4& rhs) const { return Vector4(x / rhs.x, y / rhs.y, z / rhs.z, w / rhs.w); }

        /// Add-assign a vector.
        Vector4& operator+=(const Vector4& rhs)
        {
            x += rhs.x;
            y += rhs.y;
            z += rhs.z;
            w += rhs.w;
            return *this;
        }

        /// Subtract-assign a vector.
        Vector4& operator-=(const Vector4& rhs)
        {
            x -= rhs.x;
            y -= rhs.y;
            z -= rhs.z;
            w -= rhs.w;
            return *this;
        }

        /// Multiply-assign a scalar.
        Vector4& operator*=(float rhs)
        {
            x *= rhs;
            y *= rhs;
            z *= rhs;
            w *= rhs;
            return *this;
        }

        /// Multiply-assign a vector.
        Vector4& operator*=(const Vector4& rhs)
        {
            x *= rhs.x;
            y *= rhs.y;
            z *= rhs.z;
            w *= rhs.w;
            return *this;
        }

        /// Divide-assign a scalar.
        Vector4& operator/=(float rhs)
        {
            float invRhs = 1.0f / rhs;
            x *= invRhs;
            y *= invRhs;
            z *= invRhs;
            w *= invRhs;
            return *this;
        }

        /// Divide-assign a vector.
        Vector4& operator/=(const Vector4& rhs)
        {
            x /= rhs.x;
            y /= rhs.y;
            z /= rhs.z;
            w /= rhs.w;
            return *this;
        }

        inline float operator[](int i) const
        {
            switch(i)
            {
            case 0:
                return x;
            case 1:
                return y;
            case 2:
                return z;
            case 3:
                return w;
            default:
                return 0.0f;
            }
        }

        inline Vector4 operator+(float v) const { return Vector4(x + v, y + v, z + v, w + v); }
        inline Vector4 operator-(float v) const { return Vector4(x - v, y - v, z - v, w - v); }
        inline void operator+=(float v)
        {
            x += v;
            y += v;
            z += v;
            w += v;
        }
        inline void operator-=(float v)
        {
            x -= v;
            y -= v;
            z -= v;
            w -= v;
        }

        Vector3 ToVector3() const
        {
            return Vector3(x, y, z);
        }

        /// Return const value by index.
        float operator[](unsigned index) const { return (&x)[index]; }

        /// Return mutable value by index.
        float& operator[](unsigned index) { return (&x)[index]; }

        /// Calculate dot product.
        float DotProduct(const Vector4& rhs) const { return x * rhs.x + y * rhs.y + z * rhs.z + w * rhs.w; }

        /// Calculate absolute dot product.
        float AbsDotProduct(const Vector4& rhs) const
        {
            return Lumos::Maths::Abs(x * rhs.x) + Lumos::Maths::Abs(y * rhs.y) + Lumos::Maths::Abs(z * rhs.z) + Lumos::Maths::Abs(w * rhs.w);
        }

        /// Project vector onto axis.
        float ProjectOntoAxis(const Vector3& axis) const { return DotProduct(Vector4(axis.Normalised(), 0.0f)); }

        /// Return absolute vector.
        Vector4 Abs() const { return Vector4(Lumos::Maths::Abs(x), Lumos::Maths::Abs(y), Lumos::Maths::Abs(z), Lumos::Maths::Abs(w)); }

        /// Linear interpolation with another vector.
        Vector4 Lerp(const Vector4& rhs, float t) const { return *this * (1.0f - t) + rhs * t; }

        /// Test for equality with another vector with epsilon.
        bool Equals(const Vector4& rhs, float eps = M_EPSILON) const
        {
            return Lumos::Maths::Equals(x, rhs.x, eps) && Lumos::Maths::Equals(y, rhs.y, eps) && Lumos::Maths::Equals(z, rhs.z, eps) && Lumos::Maths::Equals(w, rhs.w, eps);
        }

        /// Return whether is NaN.
        bool IsNaN() const { return Lumos::Maths::IsNaN(x) || Lumos::Maths::IsNaN(y) || Lumos::Maths::IsNaN(z) || Lumos::Maths::IsNaN(w); }

        /// Return float data.
        const float* Data() const { return &x; }

        /// Return as string.

        /// Return hash value for HashSet & HashMap.
        unsigned ToHash() const
        {
            unsigned hash = 37;
            hash = 37 * hash + FloatToRawIntBits(x);
            hash = 37 * hash + FloatToRawIntBits(y);
            hash = 37 * hash + FloatToRawIntBits(z);
            hash = 37 * hash + FloatToRawIntBits(w);

            return hash;
        }

        friend std::ostream& operator<<(std::ostream& o, const Vector4& v) { return o << "Vector4(" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ")"; }

        /// X coordinate.
        float x;
        /// Y coordinate.
        float y;
        /// Z coordinate.
        float z;
        /// W coordinate.
        float w;

        /// Zero vector.
        static const Vector4 ZERO;
        /// (1,1,1) vector.
        static const Vector4 ONE;

        static float Dot(const Vector4& a, const Vector4& b)
        {
            return a.DotProduct(b);
        }
    };

    /// Multiply Vector4 with a scalar.
    inline Vector4 operator*(float lhs, const Vector4& rhs) { return rhs * lhs; }

    /// Per-component linear interpolation between two 4-vectors.
    inline Vector4 VectorLerp(const Vector4& lhs, const Vector4& rhs, const Vector4& t) { return lhs + (rhs - lhs) * t; }

    /// Per-component min of two 4-vectors.
    inline Vector4 VectorMin(const Vector4& lhs, const Vector4& rhs) { return Vector4(Min(lhs.x, rhs.x), Min(lhs.y, rhs.y), Min(lhs.z, rhs.z), Min(lhs.w, rhs.w)); }

    /// Per-component max of two 4-vectors.
    inline Vector4 VectorMax(const Vector4& lhs, const Vector4& rhs) { return Vector4(Max(lhs.x, rhs.x), Max(lhs.y, rhs.y), Max(lhs.z, rhs.z), Max(lhs.w, rhs.w)); }

    /// Per-component floor of 4-vector.
    inline Vector4 VectorFloor(const Vector4& vec) { return Vector4(Floor(vec.x), Floor(vec.y), Floor(vec.z), Floor(vec.w)); }

    /// Per-component round of 4-vector.
    inline Vector4 VectorRound(const Vector4& vec) { return Vector4(Round(vec.x), Round(vec.y), Round(vec.z), Round(vec.w)); }

    /// Per-component ceil of 4-vector.
    inline Vector4 VectorCeil(const Vector4& vec) { return Vector4(Ceil(vec.x), Ceil(vec.y), Ceil(vec.z), Ceil(vec.w)); }

    inline Vector4 operator+(float f, const Vector4& v) { return v + f; }

    inline Vector4 operator-(float f, const Vector4& v) { return Vector4(f) - v; }
    inline Vector4 operator/(float f, const Vector4& v) { return Vector4(f) / v; }

    template <typename Archive>
    void serialize(Archive& archive, Maths::Vector4& v4)
    {
        archive(v4.x, v4.y, v4.z, v4.w);
    }
}

namespace std
{
    template <>
    struct hash<Lumos::Maths::Vector4>
    {
        size_t operator()(const Lumos::Maths::Vector4& x) const noexcept
        {
            return hash<float>()(x.x) ^ (hash<float>()(x.y) * 997u) ^ (hash<float>()(x.z) * 999983u) ^ (hash<float>()(x.w) * 999999937);
        }
    };
}
