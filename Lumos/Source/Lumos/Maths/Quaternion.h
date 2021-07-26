#pragma once
#include "Maths/Matrix3.h"

#ifdef LUMOS_SSE
#include <emmintrin.h>
#endif

namespace Lumos::Maths
{
    class Matrix4;
    /// Rotation represented as a four-dimensional Normalised vector.
    class Quaternion
    {
    public:
        /// Construct an identity quaternion.
        Quaternion() noexcept
#ifndef LUMOS_SSE
            : w(1.0f)
            , x(0.0f)
            , y(0.0f)
            , z(0.0f)
#endif
        {
#ifdef LUMOS_SSE
            _mm_storeu_ps(&w, _mm_set_ps(0.f, 0.f, 0.f, 1.f));
#endif
        }

        /// Copy-construct from another quaternion.
        Quaternion(const Quaternion& quat) noexcept
#if defined(LUMOS_SSE) && (!defined(_MSC_VER) || _MSC_VER >= 1700) /* Visual Studio 2012 and newer. VS2010 bug */
        {
            _mm_storeu_ps(&w, _mm_loadu_ps(&quat.w));
        }
#else
            : w(quat.w)
            , x(quat.x)
            , y(quat.y)
            , z(quat.z)
        {
        }
#endif

        /// Construct from values.
        Quaternion(float pw, float px, float py, float pz) noexcept
#ifndef LUMOS_SSE
            : w(pw)
            , x(px)
            , y(py)
            , z(pz)
#endif
        {
#ifdef LUMOS_SSE
            _mm_storeu_ps(&w, _mm_set_ps(pz, py, px, pw));
#endif
        }

        /// Construct from a float array.
        explicit Quaternion(const float* data) noexcept
#ifndef LUMOS_SSE
            : w(data[0])
            , x(data[1])
            , y(data[2])
            , z(data[3])
#endif
        {
#ifdef LUMOS_SSE
            _mm_storeu_ps(&w, _mm_loadu_ps(data));
#endif
        }

        /// Construct from an angle (in degrees) and axis.
        Quaternion(float angle, const Vector3& axis) noexcept
        {
            FromAngleAxis(angle, axis);
        }

        /// Construct from an angle in degrees
        explicit Quaternion(float angle) noexcept
        {
            FromAngleAxis(angle, Vector3::FORWARD);
        }

        /// Construct from Euler angles (in degrees.) Equivalent to Y*X*Z.
        Quaternion(float x, float y, float z) noexcept
        {
            FromEulerAngles(x, y, z);
        }

        /// Construct from Euler angles (in degrees.)
        explicit Quaternion(Vector3 angles) noexcept
        {
            FromEulerAngles(angles.x, angles.y, angles.z);
        }

        /// Construct from the rotation difference between two direction vectors.
        Quaternion(const Vector3& start, const Vector3& end) noexcept
        {
            FromRotationTo(start, end);
        }

        /// Construct from orthonormal axes.
        Quaternion(const Vector3& xAxis, const Vector3& yAxis, const Vector3& zAxis) noexcept
        {
            FromAxes(xAxis, yAxis, zAxis);
        }

        /// Construct from a rotation matrix.
        explicit Quaternion(const Matrix3& matrix) noexcept
        {
            FromRotationMatrix(matrix);
        }

#ifdef LUMOS_SSE
        explicit Quaternion(__m128 wxyz) noexcept
        {
            _mm_storeu_ps(&w, wxyz);
        }
#endif

        /// Assign from another quaternion.
        Quaternion& operator=(const Quaternion& rhs) noexcept
        {
#if defined(LUMOS_SSE)
            _mm_storeu_ps(&w, _mm_loadu_ps(&rhs.w));
#else
            w = rhs.w;
            x = rhs.x;
            y = rhs.y;
            z = rhs.z;
#endif
            return *this;
        }

        /// Add-assign a quaternion.
        Quaternion& operator+=(const Quaternion& rhs)
        {
#ifdef LUMOS_SSE
            _mm_storeu_ps(&w, _mm_add_ps(_mm_loadu_ps(&w), _mm_loadu_ps(&rhs.w)));
#else
            w += rhs.w;
            x += rhs.x;
            y += rhs.y;
            z += rhs.z;
#endif
            return *this;
        }

        /// Multiply-assign a scalar.
        Quaternion& operator*=(float rhs)
        {
#ifdef LUMOS_SSE
            _mm_storeu_ps(&w, _mm_mul_ps(_mm_loadu_ps(&w), _mm_set1_ps(rhs)));
#else
            w *= rhs;
            x *= rhs;
            y *= rhs;
            z *= rhs;
#endif
            return *this;
        }

        /// Test for equality with another quaternion without epsilon.
        bool operator==(const Quaternion& rhs) const
        {
#ifdef LUMOS_SSE
            __m128 c = _mm_cmpeq_ps(_mm_loadu_ps(&w), _mm_loadu_ps(&rhs.w));
            c = _mm_and_ps(c, _mm_movehl_ps(c, c));
            c = _mm_and_ps(c, _mm_shuffle_ps(c, c, _MM_SHUFFLE(1, 1, 1, 1)));
            return _mm_cvtsi128_si32(_mm_castps_si128(c)) == -1;
#else
            return w == rhs.w && x == rhs.x && y == rhs.y && z == rhs.z;
#endif
        }

        /// Test for inequality with another quaternion without epsilon.
        bool operator!=(const Quaternion& rhs) const { return !(*this == rhs); }

        /// Multiply with a scalar.
        Quaternion operator*(float rhs) const
        {
#ifdef LUMOS_SSE
            return Quaternion(_mm_mul_ps(_mm_loadu_ps(&w), _mm_set1_ps(rhs)));
#else
            return Quaternion(w * rhs, x * rhs, y * rhs, z * rhs);
#endif
        }

        /// Return negation.
        Quaternion operator-() const
        {
#ifdef LUMOS_SSE
            return Quaternion(_mm_xor_ps(_mm_loadu_ps(&w), _mm_castsi128_ps(_mm_set1_epi32((int)0x80000000UL))));
#else
            return Quaternion(-w, -x, -y, -z);
#endif
        }

        /// Add a quaternion.
        Quaternion operator+(const Quaternion& rhs) const
        {
#ifdef LUMOS_SSE
            return Quaternion(_mm_add_ps(_mm_loadu_ps(&w), _mm_loadu_ps(&rhs.w)));
#else
            return Quaternion(w + rhs.w, x + rhs.x, y + rhs.y, z + rhs.z);
#endif
        }

        /// Subtract a quaternion.
        Quaternion operator-(const Quaternion& rhs) const
        {
#ifdef LUMOS_SSE
            return Quaternion(_mm_sub_ps(_mm_loadu_ps(&w), _mm_loadu_ps(&rhs.w)));
#else
            return Quaternion(w - rhs.w, x - rhs.x, y - rhs.y, z - rhs.z);
#endif
        }

        /// Multiply a quaternion.
        Quaternion operator*(const Quaternion& rhs) const
        {
#ifdef LUMOS_SSE
            __m128 q1 = _mm_loadu_ps(&w);
            __m128 q2 = _mm_loadu_ps(&rhs.w);
            q2 = _mm_shuffle_ps(q2, q2, _MM_SHUFFLE(0, 3, 2, 1));
            const __m128 signy = _mm_castsi128_ps(_mm_set_epi32((int)0x80000000UL, (int)0x80000000UL, 0, 0));
            const __m128 signx = _mm_shuffle_ps(signy, signy, _MM_SHUFFLE(2, 0, 2, 0));
            const __m128 signz = _mm_shuffle_ps(signy, signy, _MM_SHUFFLE(3, 0, 0, 3));
            __m128 out = _mm_mul_ps(_mm_shuffle_ps(q1, q1, _MM_SHUFFLE(1, 1, 1, 1)), _mm_shuffle_ps(q2, q2, _MM_SHUFFLE(0, 1, 2, 3)));
            out = _mm_add_ps(_mm_mul_ps(_mm_xor_ps(signy, _mm_shuffle_ps(q1, q1, _MM_SHUFFLE(2, 2, 2, 2))), _mm_shuffle_ps(q2, q2, _MM_SHUFFLE(1, 0, 3, 2))), _mm_xor_ps(signx, out));
            out = _mm_add_ps(_mm_mul_ps(_mm_xor_ps(signz, _mm_shuffle_ps(q1, q1, _MM_SHUFFLE(3, 3, 3, 3))), _mm_shuffle_ps(q2, q2, _MM_SHUFFLE(2, 3, 0, 1))), out);
            out = _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(q1, q1, _MM_SHUFFLE(0, 0, 0, 0)), q2), out);
            return Quaternion(_mm_shuffle_ps(out, out, _MM_SHUFFLE(2, 1, 0, 3)));
#else
            return Quaternion(
                w * rhs.w - x * rhs.x - y * rhs.y - z * rhs.z,
                w * rhs.x + x * rhs.w + y * rhs.z - z * rhs.y,
                w * rhs.y + y * rhs.w + z * rhs.x - x * rhs.z,
                w * rhs.z + z * rhs.w + x * rhs.y - y * rhs.x);
#endif
        }

        /// Multiply a Vector3.
        Vector3 operator*(const Vector3& rhs) const
        {
#ifdef LUMOS_SSE
            __m128 q = _mm_loadu_ps(&w);
            q = _mm_shuffle_ps(q, q, _MM_SHUFFLE(0, 3, 2, 1));
            __m128 v = _mm_set_ps(0.f, rhs.z, rhs.y, rhs.x);
            const __m128 W = _mm_shuffle_ps(q, q, _MM_SHUFFLE(3, 3, 3, 3));
            const __m128 a_yzx = _mm_shuffle_ps(q, q, _MM_SHUFFLE(3, 0, 2, 1));
            __m128 x = _mm_mul_ps(q, _mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 0, 2, 1)));
            __m128 qxv = _mm_sub_ps(x, _mm_mul_ps(a_yzx, v));
            __m128 Wv = _mm_mul_ps(W, v);
            __m128 s = _mm_add_ps(qxv, _mm_shuffle_ps(Wv, Wv, _MM_SHUFFLE(3, 1, 0, 2)));
            __m128 qs = _mm_mul_ps(q, s);
            __m128 y = _mm_shuffle_ps(qs, qs, _MM_SHUFFLE(3, 1, 0, 2));
            s = _mm_sub_ps(_mm_mul_ps(a_yzx, s), y);
            s = _mm_add_ps(s, s);
            s = _mm_add_ps(s, v);

            return Vector3(
                _mm_cvtss_f32(s),
                _mm_cvtss_f32(_mm_shuffle_ps(s, s, _MM_SHUFFLE(1, 1, 1, 1))),
                _mm_cvtss_f32(_mm_movehl_ps(s, s)));
#else
            Vector3 qVec(x, y, z);
            Vector3 cross1(qVec.CrossProduct(rhs));
            Vector3 cross2(qVec.CrossProduct(cross1));

            return rhs + 2.0f * (cross1 * w + cross2);
#endif
        }

        /// Define from an angle (in degrees) and axis.
        void FromAngleAxis(float angle, const Vector3& axis);
        /// Define from Euler angles (in degrees.) Equivalent to Y*X*Z.
        void FromEulerAngles(float pitch, float yaw, float roll);
        /// Define from the rotation difference between two direction vectors.
        void FromRotationTo(const Vector3& start, const Vector3& end);
        /// Define from orthonormal axes.
        void FromAxes(const Vector3& xAxis, const Vector3& yAxis, const Vector3& zAxis);
        /// Define from a rotation matrix.
        void FromRotationMatrix(const Matrix3& matrix);
        /// Define from a direction to look in and an up direction. Return true if successful, or false if would result in a NaN, in which case the current value remains.
        bool FromLookRotation(const Vector3& direction, const Vector3& up = Vector3::UP);

        /// Normalise to unit length.
        void Normalise()
        {
#ifdef LUMOS_SSE
            __m128 q = _mm_loadu_ps(&w);
            __m128 n = _mm_mul_ps(q, q);
            n = _mm_add_ps(n, _mm_shuffle_ps(n, n, _MM_SHUFFLE(2, 3, 0, 1)));
            n = _mm_add_ps(n, _mm_shuffle_ps(n, n, _MM_SHUFFLE(0, 1, 2, 3)));
            __m128 e = _mm_rsqrt_ps(n);
            __m128 e3 = _mm_mul_ps(_mm_mul_ps(e, e), e);
            __m128 half = _mm_set1_ps(0.5f);
            n = _mm_add_ps(e, _mm_mul_ps(half, _mm_sub_ps(e, _mm_mul_ps(n, e3))));
            _mm_storeu_ps(&w, _mm_mul_ps(q, n));
#else
            float lenSquared = LengthSquared();
            if(!Lumos::Maths::Equals(lenSquared, 1.0f) && lenSquared > 0.0f)
            {
                float invLen = 1.0f / sqrtf(lenSquared);
                w *= invLen;
                x *= invLen;
                y *= invLen;
                z *= invLen;
            }
#endif
        }

        /// Return Normalised to unit length.
        Quaternion Normalised() const
        {
#ifdef LUMOS_SSE
            __m128 q = _mm_loadu_ps(&w);
            __m128 n = _mm_mul_ps(q, q);
            n = _mm_add_ps(n, _mm_shuffle_ps(n, n, _MM_SHUFFLE(2, 3, 0, 1)));
            n = _mm_add_ps(n, _mm_shuffle_ps(n, n, _MM_SHUFFLE(0, 1, 2, 3)));
            __m128 e = _mm_rsqrt_ps(n);
            __m128 e3 = _mm_mul_ps(_mm_mul_ps(e, e), e);
            __m128 half = _mm_set1_ps(0.5f);
            n = _mm_add_ps(e, _mm_mul_ps(half, _mm_sub_ps(e, _mm_mul_ps(n, e3))));
            return Quaternion(_mm_mul_ps(q, n));
#else
            float lenSquared = LengthSquared();
            if(!Lumos::Maths::Equals(lenSquared, 1.0f) && lenSquared > 0.0f)
            {
                float invLen = 1.0f / sqrtf(lenSquared);
                return *this * invLen;
            }
            else
                return *this;
#endif
        }

        /// Return inverse.
        Quaternion Inverse() const
        {
#ifdef LUMOS_SSE
            __m128 q = _mm_loadu_ps(&w);
            __m128 n = _mm_mul_ps(q, q);
            n = _mm_add_ps(n, _mm_shuffle_ps(n, n, _MM_SHUFFLE(2, 3, 0, 1)));
            n = _mm_add_ps(n, _mm_shuffle_ps(n, n, _MM_SHUFFLE(0, 1, 2, 3)));
            return Quaternion(_mm_div_ps(_mm_xor_ps(q, _mm_castsi128_ps(_mm_set_epi32((int)0x80000000UL, (int)0x80000000UL, (int)0x80000000UL, 0))), n));
#else
            float lenSquared = LengthSquared();
            if(lenSquared == 1.0f)
                return Conjugate();
            else if(lenSquared >= M_EPSILON)
                return Conjugate() * (1.0f / lenSquared);
            else
                return IDENTITY;
#endif
        }

        /// Return squared length.
        float LengthSquared() const
        {
#ifdef LUMOS_SSE
            __m128 q = _mm_loadu_ps(&w);
            __m128 n = _mm_mul_ps(q, q);
            n = _mm_add_ps(n, _mm_shuffle_ps(n, n, _MM_SHUFFLE(2, 3, 0, 1)));
            n = _mm_add_ps(n, _mm_shuffle_ps(n, n, _MM_SHUFFLE(0, 1, 2, 3)));
            return _mm_cvtss_f32(n);
#else
            return w * w + x * x + y * y + z * z;
#endif
        }

        /// Calculate dot product.
        float DotProduct(const Quaternion& rhs) const
        {
#ifdef LUMOS_SSE
            __m128 q1 = _mm_loadu_ps(&w);
            __m128 q2 = _mm_loadu_ps(&rhs.w);
            __m128 n = _mm_mul_ps(q1, q2);
            n = _mm_add_ps(n, _mm_shuffle_ps(n, n, _MM_SHUFFLE(2, 3, 0, 1)));
            n = _mm_add_ps(n, _mm_shuffle_ps(n, n, _MM_SHUFFLE(0, 1, 2, 3)));
            return _mm_cvtss_f32(n);
#else
            return w * rhs.w + x * rhs.x + y * rhs.y + z * rhs.z;
#endif
        }

        /// Test for equality with another quaternion with epsilon.
        bool Equals(const Quaternion& rhs, float eps = M_EPSILON) const
        {
            return Lumos::Maths::Equals(w, rhs.w, eps) && Lumos::Maths::Equals(x, rhs.x, eps) && Lumos::Maths::Equals(y, rhs.y, eps) && Lumos::Maths::Equals(z, rhs.z, eps);
        }

        /// Return whether any element is NaN.
        bool IsNaN() const { return Lumos::Maths::IsNaN(w) || Lumos::Maths::IsNaN(x) || Lumos::Maths::IsNaN(y) || Lumos::Maths::IsNaN(z); }

        /// Return whether any element is Inf.
        bool IsInf() const { return Lumos::Maths::IsInf(w) || Lumos::Maths::IsInf(x) || Lumos::Maths::IsInf(y) || Lumos::Maths::IsInf(z); }

        /// Return conjugate.
        Quaternion Conjugate() const
        {
#ifdef LUMOS_SSE
            __m128 q = _mm_loadu_ps(&w);
            return Quaternion(_mm_xor_ps(q, _mm_castsi128_ps(_mm_set_epi32((int)0x80000000UL, (int)0x80000000UL, (int)0x80000000UL, 0))));
#else
            return Quaternion(w, -x, -y, -z);
#endif
        }

        /// Return Euler angles in degrees.
        Vector3 EulerAngles() const;
        /// Return yaw angle in degrees.
        float YawAngle() const;
        /// Return pitch angle in degrees.
        float PitchAngle() const;
        /// Return roll angle in degrees.
        float RollAngle() const;
        /// Return rotation axis.
        Vector3 Axis() const;
        /// Return rotation angle.
        float Angle() const;
        /// Return the rotation matrix that corresponds to this quaternion.
        Matrix3 RotationMatrix() const;

        Matrix4 RotationMatrix4() const;
        /// Spherical interpolation with another quaternion.
        Quaternion Slerp(const Quaternion& rhs, float t) const;
        /// Normalised linear interpolation with another quaternion.
        Quaternion Nlerp(const Quaternion& rhs, float t, bool shortestPath = false) const;

        /// Return float data.
        const float* Data() const { return &w; }

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

        /// W coordinate.
        float w = 0.0f;
        /// X coordinate.
        float x = 0.0f;
        /// Y coordinate.
        float y = 0.0f;
        /// Z coordinate.
        float z = 0.0f;

        /// Identity quaternion.
        static const Quaternion IDENTITY;

        static Quaternion EulerAnglesToQuaternion(float x, float y, float z)
        {
            Quaternion q;
            q.FromEulerAngles(x, y, z);
            return q;
        }

        static Quaternion LookAt(const Vector3& from, const Vector3& to, const Vector3& up = Vector3(0.0f, 1.0f, 0.0f))
        {
            Quaternion q;
            q.FromLookRotation(from - to, up);
            return q;
        }
    };

    template <typename Archive>
    void serialize(Archive& archive, Maths::Quaternion& v4)
    {
        archive(v4.x, v4.y, v4.z, v4.w);
    }

    inline Quaternion operator*(const Vector3& v, const Quaternion& rhs)
    {
        return Quaternion(
            -(rhs.x * v.x) - (rhs.y * v.y) - (rhs.z * v.z),
            (rhs.w * v.x) + (v.y * rhs.z) - (v.z * rhs.y),
            (rhs.w * v.y) + (v.z * rhs.x) - (v.x * rhs.z),
            (rhs.w * v.z) + (v.x * rhs.y) - (v.y * rhs.x));
    }
}
