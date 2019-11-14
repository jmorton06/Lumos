//
// Copyright (c) 2008-2019 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#pragma once

#include "../Math/Vector3.h"

namespace Urho3D
{

/// Four-dimensional vector.
class  Vector4
{
public:
    /// Construct a zero vector.
    Vector4() noexcept :
        x(0.0f),
        y(0.0f),
        z(0.0f),
        w(0.0f)
    {
    }

    /// Copy-construct from another vector.
    Vector4(const Vector4& vector) noexcept = default;

    /// Construct from a 3-dimensional vector and the W coordinate.
    Vector4(const Vector3& vector, float w) noexcept :
        x(vector.x),
        y(vector.y),
        z(vector.z),
        w(w)
    {
    }

    /// Construct from coordinates.
    Vector4(float x, float y, float z, float w) noexcept :
        x(x),
        y(y),
        z(z),
        w(w)
    {
    }

    /// Construct from a float array.
    explicit Vector4(const float* data) noexcept :
        x(data[0]),
        y(data[1]),
        z(data[2]),
        w(data[3])
    {
    }

    /// Assign from another vector.
    Vector4& operator =(const Vector4& rhs) noexcept = default;

    /// Test for equality with another vector without epsilon.
    bool operator ==(const Vector4& rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w; }

    /// Test for inequality with another vector without epsilon.
    bool operator !=(const Vector4& rhs) const { return x != rhs.x || y != rhs.y || z != rhs.z || w != rhs.w; }

    /// Add a vector.
    Vector4 operator +(const Vector4& rhs) const { return Vector4(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w); }

    /// Return negation.
    Vector4 operator -() const { return Vector4(-x, -y, -z, -w); }

    /// Subtract a vector.
    Vector4 operator -(const Vector4& rhs) const { return Vector4(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w); }

    /// Multiply with a scalar.
    Vector4 operator *(float rhs) const { return Vector4(x * rhs, y * rhs, z * rhs, w * rhs); }

    /// Multiply with a vector.
    Vector4 operator *(const Vector4& rhs) const { return Vector4(x * rhs.x, y * rhs.y, z * rhs.z, w * rhs.w); }

    /// Divide by a scalar.
    Vector4 operator /(float rhs) const { return Vector4(x / rhs, y / rhs, z / rhs, w / rhs); }

    /// Divide by a vector.
    Vector4 operator /(const Vector4& rhs) const { return Vector4(x / rhs.x, y / rhs.y, z / rhs.z, w / rhs.w); }

    /// Add-assign a vector.
    Vector4& operator +=(const Vector4& rhs)
    {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        w += rhs.w;
        return *this;
    }

    /// Subtract-assign a vector.
    Vector4& operator -=(const Vector4& rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        w -= rhs.w;
        return *this;
    }

    /// Multiply-assign a scalar.
    Vector4& operator *=(float rhs)
    {
        x *= rhs;
        y *= rhs;
        z *= rhs;
        w *= rhs;
        return *this;
    }

    /// Multiply-assign a vector.
    Vector4& operator *=(const Vector4& rhs)
    {
        x *= rhs.x;
        y *= rhs.y;
        z *= rhs.z;
        w *= rhs.w;
        return *this;
    }

    /// Divide-assign a scalar.
    Vector4& operator /=(float rhs)
    {
        float invRhs = 1.0f / rhs;
        x *= invRhs;
        y *= invRhs;
        z *= invRhs;
        w *= invRhs;
        return *this;
    }

    /// Divide-assign a vector.
    Vector4& operator /=(const Vector4& rhs)
    {
        x /= rhs.x;
        y /= rhs.y;
        z /= rhs.z;
        w /= rhs.w;
        return *this;
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
        return Urho3D::Abs(x * rhs.x) + Urho3D::Abs(y * rhs.y) + Urho3D::Abs(z * rhs.z) + Urho3D::Abs(w * rhs.w);
    }

    /// Project vector onto axis.
    float ProjectOntoAxis(const Vector3& axis) const { return DotProduct(Vector4(axis.Normalized(), 0.0f)); }

    /// Return absolute vector.
    Vector4 Abs() const { return Vector4(Urho3D::Abs(x), Urho3D::Abs(y), Urho3D::Abs(z), Urho3D::Abs(w)); }

    /// Linear interpolation with another vector.
    Vector4 Lerp(const Vector4& rhs, float t) const { return *this * (1.0f - t) + rhs * t; }

    /// Test for equality with another vector with epsilon.
    bool Equals(const Vector4& rhs, float eps = M_EPSILON) const
    {
        return Urho3D::Equals(x, rhs.x, eps) && Urho3D::Equals(y, rhs.y, eps) && Urho3D::Equals(z, rhs.z, eps) && Urho3D::Equals(w, rhs.w, eps);
    }

    /// Return whether is NaN.
    bool IsNaN() const { return Urho3D::IsNaN(x) || Urho3D::IsNaN(y) || Urho3D::IsNaN(z) || Urho3D::IsNaN(w); }

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
};

/// Multiply Vector4 with a scalar.
inline Vector4 operator *(float lhs, const Vector4& rhs) { return rhs * lhs; }

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

}
