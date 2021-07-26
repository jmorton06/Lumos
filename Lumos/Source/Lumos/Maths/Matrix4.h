#pragma once

#include "Maths/Quaternion.h"
#include "Maths/Vector4.h"
#include "Maths/Matrix3.h"

#include <iostream>
#ifdef LUMOS_SSE
#include <emmintrin.h>
#endif

#define CLIP_CONTROL_ZO_BIT (1 << 0) // ZERO_TO_ONE
#define CLIP_CONTROL_NO_BIT (1 << 1) // NEGATIVE_ONE_TO_ONE
#define CLIP_CONTROL_LH_BIT (1 << 2) // LEFT_HANDED, For DirectX, Metal, Vulkan
#define CLIP_CONTROL_RH_BIT (1 << 3) // RIGHT_HANDED, For OpenGL, default

#define CLIP_CONTROL_LH_ZO (CLIP_CONTROL_LH_BIT | CLIP_CONTROL_ZO_BIT)
#define CLIP_CONTROL_LH_NO (CLIP_CONTROL_LH_BIT | CLIP_CONTROL_NO_BIT)
#define CLIP_CONTROL_RH_ZO (CLIP_CONTROL_RH_BIT | CLIP_CONTROL_ZO_BIT)
#define CLIP_CONTROL_RH_NO (CLIP_CONTROL_RH_BIT | CLIP_CONTROL_NO_BIT)

namespace Lumos::Maths
{
    class Matrix4
    {
    public:
        static int CONFIG_CLIP_CONTROL;
        static void SetUpCoordSystem(bool LeftHanded, bool forceZeroToOne);
        static bool IsDepthZeroOne();

        /// Construct an identity matrix.
        Matrix4() noexcept
#ifndef LUMOS_SSE
            : m00_(1.0f)
            , m01_(0.0f)
            , m02_(0.0f)
            , m03_(0.0f)
            , m10_(0.0f)
            , m11_(1.0f)
            , m12_(0.0f)
            , m13_(0.0f)
            , m20_(0.0f)
            , m21_(0.0f)
            , m22_(1.0f)
            , m23_(0.0f)
            , m30_(0.0f)
            , m31_(0.0f)
            , m32_(0.0f)
            , m33_(1.0f)
#endif
        {
#ifdef LUMOS_SSE
            _mm_storeu_ps(&m00_, _mm_set_ps(0.f, 0.f, 0.f, 1.f));
            _mm_storeu_ps(&m10_, _mm_set_ps(0.f, 0.f, 1.f, 0.f));
            _mm_storeu_ps(&m20_, _mm_set_ps(0.f, 1.f, 0.f, 0.f));
            _mm_storeu_ps(&m30_, _mm_set_ps(1.f, 0.f, 0.f, 0.f));
#endif
        }

        /// Copy-construct from another matrix.
        Matrix4(const Matrix4& matrix) noexcept
#ifndef LUMOS_SSE
            : m00_(matrix.m00_)
            , m01_(matrix.m01_)
            , m02_(matrix.m02_)
            , m03_(matrix.m03_)
            , m10_(matrix.m10_)
            , m11_(matrix.m11_)
            , m12_(matrix.m12_)
            , m13_(matrix.m13_)
            , m20_(matrix.m20_)
            , m21_(matrix.m21_)
            , m22_(matrix.m22_)
            , m23_(matrix.m23_)
            , m30_(matrix.m30_)
            , m31_(matrix.m31_)
            , m32_(matrix.m32_)
            , m33_(matrix.m33_)
#endif
        {
#ifdef LUMOS_SSE
            _mm_storeu_ps(&m00_, _mm_loadu_ps(&matrix.m00_));
            _mm_storeu_ps(&m10_, _mm_loadu_ps(&matrix.m10_));
            _mm_storeu_ps(&m20_, _mm_loadu_ps(&matrix.m20_));
            _mm_storeu_ps(&m30_, _mm_loadu_ps(&matrix.m30_));
#endif
        }

        /// Copy-construct from a 3x3 matrix and set the extra elements to identity.
        explicit Matrix4(const Matrix3& matrix) noexcept
            : m00_(matrix.m00_)
            , m01_(matrix.m01_)
            , m02_(matrix.m02_)
            , m03_(0.0f)
            , m10_(matrix.m10_)
            , m11_(matrix.m11_)
            , m12_(matrix.m12_)
            , m13_(0.0f)
            , m20_(matrix.m20_)
            , m21_(matrix.m21_)
            , m22_(matrix.m22_)
            , m23_(0.0f)
            , m30_(0.0f)
            , m31_(0.0f)
            , m32_(0.0f)
            , m33_(1.0f)
        {
        }

        /// Construct from values.
        Matrix4(float v00, float v01, float v02, float v03,
            float v10, float v11, float v12, float v13,
            float v20, float v21, float v22, float v23,
            float v30, float v31, float v32, float v33) noexcept
            : m00_(v00)
            , m01_(v01)
            , m02_(v02)
            , m03_(v03)
            , m10_(v10)
            , m11_(v11)
            , m12_(v12)
            , m13_(v13)
            , m20_(v20)
            , m21_(v21)
            , m22_(v22)
            , m23_(v23)
            , m30_(v30)
            , m31_(v31)
            , m32_(v32)
            , m33_(v33)
        {
        }

        /// Construct from a float array.
        explicit Matrix4(const float* data) noexcept
#ifndef LUMOS_SSE
            : m00_(data[0])
            , m01_(data[1])
            , m02_(data[2])
            , m03_(data[3])
            , m10_(data[4])
            , m11_(data[5])
            , m12_(data[6])
            , m13_(data[7])
            , m20_(data[8])
            , m21_(data[9])
            , m22_(data[10])
            , m23_(data[11])
            , m30_(data[12])
            , m31_(data[13])
            , m32_(data[14])
            , m33_(data[15])
#endif
        {
#ifdef LUMOS_SSE
            _mm_storeu_ps(&m00_, _mm_loadu_ps(data));
            _mm_storeu_ps(&m10_, _mm_loadu_ps(data + 4));
            _mm_storeu_ps(&m20_, _mm_loadu_ps(data + 8));
            _mm_storeu_ps(&m30_, _mm_loadu_ps(data + 12));
#endif
        }
        
        Matrix4(const Vector3& translation, const Quaternion& rotation, float scale) noexcept
        {
#ifdef LUMOS_SSE
            __m128 t = _mm_set_ps(1.f, translation.z, translation.y, translation.x);
            __m128 q = _mm_loadu_ps(&rotation.w);
            __m128 s = _mm_set_ps(1.f, scale, scale, scale);
            SetFromTRS(t, q, s);
#else
            SetRotation(rotation.RotationMatrix() * scale);
            SetTranslation(translation);
#endif
        }
        
        Matrix4(const Vector3& translation, const Quaternion& rotation, const Maths::Vector3& scale) noexcept
        {
#ifdef LUMOS_SSE
            __m128 t = _mm_set_ps(1.f, translation.z, translation.y, translation.x);
            __m128 q = _mm_loadu_ps(&rotation.w);
            __m128 s = _mm_set_ps(1.f, scale.z, scale.y, scale.x);
            SetFromTRS(t, q, s);
#else
            SetRotation(rotation.RotationMatrix().Scaled(scale));
            SetTranslation(translation);
#endif
        }

        void ToIdentity()
        {
            m00_ = 1.0f;
            m01_ = 0.0f;
            m02_ = 0.0f;
            m03_ = 0.0f;
            m10_ = 0.0f;
            m11_ = 1.0f;
            m12_ = 0.0f;
            m13_ = 0.0f;
            m20_ = 0.0f;
            m21_ = 0.0f;
            m22_ = 1.0f;
            m23_ = 0.0f;
            m30_ = 0.0f;
            m31_ = 0.0f;
            m32_ = 0.0f;
            m33_ = 1.0f;
        }

        /// Assign from another matrix.
        Matrix4& operator=(const Matrix4& rhs) noexcept
        {
#ifdef LUMOS_SSE
            _mm_storeu_ps(&m00_, _mm_loadu_ps(&rhs.m00_));
            _mm_storeu_ps(&m10_, _mm_loadu_ps(&rhs.m10_));
            _mm_storeu_ps(&m20_, _mm_loadu_ps(&rhs.m20_));
            _mm_storeu_ps(&m30_, _mm_loadu_ps(&rhs.m30_));
#else
            m00_ = rhs.m00_;
            m01_ = rhs.m01_;
            m02_ = rhs.m02_;
            m03_ = rhs.m03_;
            m10_ = rhs.m10_;
            m11_ = rhs.m11_;
            m12_ = rhs.m12_;
            m13_ = rhs.m13_;
            m20_ = rhs.m20_;
            m21_ = rhs.m21_;
            m22_ = rhs.m22_;
            m23_ = rhs.m23_;
            m30_ = rhs.m30_;
            m31_ = rhs.m31_;
            m32_ = rhs.m32_;
            m33_ = rhs.m33_;
#endif
            return *this;
        }

        /// Assign from a 3x3 matrix. Set the extra elements to identity.
        Matrix4& operator=(const Matrix3& rhs) noexcept
        {
            m00_ = rhs.m00_;
            m01_ = rhs.m01_;
            m02_ = rhs.m02_;
            m03_ = 0.0f;
            m10_ = rhs.m10_;
            m11_ = rhs.m11_;
            m12_ = rhs.m12_;
            m13_ = 0.0f;
            m20_ = rhs.m20_;
            m21_ = rhs.m21_;
            m22_ = rhs.m22_;
            m23_ = 0.0f;
            m30_ = 0.0f;
            m31_ = 0.0f;
            m32_ = 0.0f;
            m33_ = 1.0f;
            return *this;
        }

        /// Test for equality with another matrix without epsilon.
        bool operator==(const Matrix4& rhs) const
        {
#ifdef LUMOS_SSE
            __m128 c0 = _mm_cmpeq_ps(_mm_loadu_ps(&m00_), _mm_loadu_ps(&rhs.m00_));
            __m128 c1 = _mm_cmpeq_ps(_mm_loadu_ps(&m10_), _mm_loadu_ps(&rhs.m10_));
            c0 = _mm_and_ps(c0, c1);
            __m128 c2 = _mm_cmpeq_ps(_mm_loadu_ps(&m20_), _mm_loadu_ps(&rhs.m20_));
            __m128 c3 = _mm_cmpeq_ps(_mm_loadu_ps(&m30_), _mm_loadu_ps(&rhs.m30_));
            c2 = _mm_and_ps(c2, c3);
            c0 = _mm_and_ps(c0, c2);
            __m128 hi = _mm_movehl_ps(c0, c0);
            c0 = _mm_and_ps(c0, hi);
            hi = _mm_shuffle_ps(c0, c0, _MM_SHUFFLE(1, 1, 1, 1));
            c0 = _mm_and_ps(c0, hi);
            return _mm_cvtsi128_si32(_mm_castps_si128(c0)) == -1;
#else
            const float* leftData = Data();
            const float* rightData = rhs.Data();

            for(unsigned i = 0; i < 16; ++i)
            {
                if(leftData[i] != rightData[i])
                    return false;
            }

            return true;
#endif
        }

        /// Test for inequality with another matrix without epsilon.
        bool operator!=(const Matrix4& rhs) const { return !(*this == rhs); }

        /// Multiply a Vector2 which is assumed to represent position.
        Vector2 operator*(const Vector2& rhs) const
        {
            float invW = 1.0f / (m30_ * rhs.x + m31_ * rhs.y + m33_);

            return Vector2(
                (m00_ * rhs.x + m01_ * rhs.y + m03_) * invW,
                (m10_ * rhs.x + m11_ * rhs.y + m13_) * invW);
        }

        /// Multiply a Vector3 which is assumed to represent position.
        Vector3 operator*(const Vector3& rhs) const
        {
#ifdef LUMOS_SSE
            __m128 vec = _mm_set_ps(1.f, rhs.z, rhs.y, rhs.x);
            __m128 r0 = _mm_mul_ps(_mm_loadu_ps(&m00_), vec);
            __m128 r1 = _mm_mul_ps(_mm_loadu_ps(&m10_), vec);
            __m128 t0 = _mm_unpacklo_ps(r0, r1);
            __m128 t1 = _mm_unpackhi_ps(r0, r1);
            t0 = _mm_add_ps(t0, t1);
            __m128 r2 = _mm_mul_ps(_mm_loadu_ps(&m20_), vec);
            __m128 r3 = _mm_mul_ps(_mm_loadu_ps(&m30_), vec);
            __m128 t2 = _mm_unpacklo_ps(r2, r3);
            __m128 t3 = _mm_unpackhi_ps(r2, r3);
            t2 = _mm_add_ps(t2, t3);
            vec = _mm_add_ps(_mm_movelh_ps(t0, t2), _mm_movehl_ps(t2, t0));
            vec = _mm_div_ps(vec, _mm_shuffle_ps(vec, vec, _MM_SHUFFLE(3, 3, 3, 3)));
            return Vector3(
                _mm_cvtss_f32(vec),
                _mm_cvtss_f32(_mm_shuffle_ps(vec, vec, _MM_SHUFFLE(1, 1, 1, 1))),
                _mm_cvtss_f32(_mm_movehl_ps(vec, vec)));
#else
            float invW = 1.0f / (m30_ * rhs.x + m31_ * rhs.y + m32_ * rhs.z + m33_);

            return Vector3(
                (m00_ * rhs.x + m01_ * rhs.y + m02_ * rhs.z + m03_) * invW,
                (m10_ * rhs.x + m11_ * rhs.y + m12_ * rhs.z + m13_) * invW,
                (m20_ * rhs.x + m21_ * rhs.y + m22_ * rhs.z + m23_) * invW);
#endif
        }

        /// Multiply a Vector4.
        Vector4 operator*(const Vector4& rhs) const
        {
#ifdef LUMOS_SSE
            __m128 vec = _mm_loadu_ps(&rhs.x);
            __m128 r0 = _mm_mul_ps(_mm_loadu_ps(&m00_), vec);
            __m128 r1 = _mm_mul_ps(_mm_loadu_ps(&m10_), vec);
            __m128 t0 = _mm_unpacklo_ps(r0, r1);
            __m128 t1 = _mm_unpackhi_ps(r0, r1);
            t0 = _mm_add_ps(t0, t1);
            __m128 r2 = _mm_mul_ps(_mm_loadu_ps(&m20_), vec);
            __m128 r3 = _mm_mul_ps(_mm_loadu_ps(&m30_), vec);
            __m128 t2 = _mm_unpacklo_ps(r2, r3);
            __m128 t3 = _mm_unpackhi_ps(r2, r3);
            t2 = _mm_add_ps(t2, t3);
            vec = _mm_add_ps(_mm_movelh_ps(t0, t2), _mm_movehl_ps(t2, t0));

            Vector4 ret;
            _mm_storeu_ps(&ret.x, vec);
            return ret;
#else
            return Vector4(
                m00_ * rhs.x + m01_ * rhs.y + m02_ * rhs.z + m03_ * rhs.w,
                m10_ * rhs.x + m11_ * rhs.y + m12_ * rhs.z + m13_ * rhs.w,
                m20_ * rhs.x + m21_ * rhs.y + m22_ * rhs.z + m23_ * rhs.w,
                m30_ * rhs.x + m31_ * rhs.y + m32_ * rhs.z + m33_ * rhs.w);
#endif
        }

        /// Add a matrix.
        Matrix4 operator+(const Matrix4& rhs) const
        {
#ifdef LUMOS_SSE
            Matrix4 ret;
            _mm_storeu_ps(&ret.m00_, _mm_add_ps(_mm_loadu_ps(&m00_), _mm_loadu_ps(&rhs.m00_)));
            _mm_storeu_ps(&ret.m10_, _mm_add_ps(_mm_loadu_ps(&m10_), _mm_loadu_ps(&rhs.m10_)));
            _mm_storeu_ps(&ret.m20_, _mm_add_ps(_mm_loadu_ps(&m20_), _mm_loadu_ps(&rhs.m20_)));
            _mm_storeu_ps(&ret.m30_, _mm_add_ps(_mm_loadu_ps(&m30_), _mm_loadu_ps(&rhs.m30_)));
            return ret;
#else
            return Matrix4(
                m00_ + rhs.m00_,
                m01_ + rhs.m01_,
                m02_ + rhs.m02_,
                m03_ + rhs.m03_,
                m10_ + rhs.m10_,
                m11_ + rhs.m11_,
                m12_ + rhs.m12_,
                m13_ + rhs.m13_,
                m20_ + rhs.m20_,
                m21_ + rhs.m21_,
                m22_ + rhs.m22_,
                m23_ + rhs.m23_,
                m30_ + rhs.m30_,
                m31_ + rhs.m31_,
                m32_ + rhs.m32_,
                m33_ + rhs.m33_);
#endif
        }

        /// Subtract a matrix.
        Matrix4 operator-(const Matrix4& rhs) const
        {
#ifdef LUMOS_SSE
            Matrix4 ret;
            _mm_storeu_ps(&ret.m00_, _mm_sub_ps(_mm_loadu_ps(&m00_), _mm_loadu_ps(&rhs.m00_)));
            _mm_storeu_ps(&ret.m10_, _mm_sub_ps(_mm_loadu_ps(&m10_), _mm_loadu_ps(&rhs.m10_)));
            _mm_storeu_ps(&ret.m20_, _mm_sub_ps(_mm_loadu_ps(&m20_), _mm_loadu_ps(&rhs.m20_)));
            _mm_storeu_ps(&ret.m30_, _mm_sub_ps(_mm_loadu_ps(&m30_), _mm_loadu_ps(&rhs.m30_)));
            return ret;
#else
            return Matrix4(
                m00_ - rhs.m00_,
                m01_ - rhs.m01_,
                m02_ - rhs.m02_,
                m03_ - rhs.m03_,
                m10_ - rhs.m10_,
                m11_ - rhs.m11_,
                m12_ - rhs.m12_,
                m13_ - rhs.m13_,
                m20_ - rhs.m20_,
                m21_ - rhs.m21_,
                m22_ - rhs.m22_,
                m23_ - rhs.m23_,
                m30_ - rhs.m30_,
                m31_ - rhs.m31_,
                m32_ - rhs.m32_,
                m33_ - rhs.m33_);
#endif
        }

        /// Multiply with a scalar.
        Matrix4 operator*(float rhs) const
        {
#ifdef LUMOS_SSE
            Matrix4 ret;
            const __m128 mul = _mm_set1_ps(rhs);
            _mm_storeu_ps(&ret.m00_, _mm_mul_ps(_mm_loadu_ps(&m00_), mul));
            _mm_storeu_ps(&ret.m10_, _mm_mul_ps(_mm_loadu_ps(&m10_), mul));
            _mm_storeu_ps(&ret.m20_, _mm_mul_ps(_mm_loadu_ps(&m20_), mul));
            _mm_storeu_ps(&ret.m30_, _mm_mul_ps(_mm_loadu_ps(&m30_), mul));
            return ret;
#else
            return Matrix4(
                m00_ * rhs,
                m01_ * rhs,
                m02_ * rhs,
                m03_ * rhs,
                m10_ * rhs,
                m11_ * rhs,
                m12_ * rhs,
                m13_ * rhs,
                m20_ * rhs,
                m21_ * rhs,
                m22_ * rhs,
                m23_ * rhs,
                m30_ * rhs,
                m31_ * rhs,
                m32_ * rhs,
                m33_ * rhs);
#endif
        }

        /// Multiply a matrix.
        Matrix4 operator*(const Matrix4& rhs) const
        {
#ifdef LUMOS_SSE
            Matrix4 out;

            __m128 r0 = _mm_loadu_ps(&rhs.m00_);
            __m128 r1 = _mm_loadu_ps(&rhs.m10_);
            __m128 r2 = _mm_loadu_ps(&rhs.m20_);
            __m128 r3 = _mm_loadu_ps(&rhs.m30_);

            __m128 l = _mm_loadu_ps(&m00_);
            __m128 t0 = _mm_mul_ps(_mm_shuffle_ps(l, l, _MM_SHUFFLE(0, 0, 0, 0)), r0);
            __m128 t1 = _mm_mul_ps(_mm_shuffle_ps(l, l, _MM_SHUFFLE(1, 1, 1, 1)), r1);
            __m128 t2 = _mm_mul_ps(_mm_shuffle_ps(l, l, _MM_SHUFFLE(2, 2, 2, 2)), r2);
            __m128 t3 = _mm_mul_ps(_mm_shuffle_ps(l, l, _MM_SHUFFLE(3, 3, 3, 3)), r3);
            _mm_storeu_ps(&out.m00_, _mm_add_ps(_mm_add_ps(t0, t1), _mm_add_ps(t2, t3)));

            l = _mm_loadu_ps(&m10_);
            t0 = _mm_mul_ps(_mm_shuffle_ps(l, l, _MM_SHUFFLE(0, 0, 0, 0)), r0);
            t1 = _mm_mul_ps(_mm_shuffle_ps(l, l, _MM_SHUFFLE(1, 1, 1, 1)), r1);
            t2 = _mm_mul_ps(_mm_shuffle_ps(l, l, _MM_SHUFFLE(2, 2, 2, 2)), r2);
            t3 = _mm_mul_ps(_mm_shuffle_ps(l, l, _MM_SHUFFLE(3, 3, 3, 3)), r3);
            _mm_storeu_ps(&out.m10_, _mm_add_ps(_mm_add_ps(t0, t1), _mm_add_ps(t2, t3)));

            l = _mm_loadu_ps(&m20_);
            t0 = _mm_mul_ps(_mm_shuffle_ps(l, l, _MM_SHUFFLE(0, 0, 0, 0)), r0);
            t1 = _mm_mul_ps(_mm_shuffle_ps(l, l, _MM_SHUFFLE(1, 1, 1, 1)), r1);
            t2 = _mm_mul_ps(_mm_shuffle_ps(l, l, _MM_SHUFFLE(2, 2, 2, 2)), r2);
            t3 = _mm_mul_ps(_mm_shuffle_ps(l, l, _MM_SHUFFLE(3, 3, 3, 3)), r3);
            _mm_storeu_ps(&out.m20_, _mm_add_ps(_mm_add_ps(t0, t1), _mm_add_ps(t2, t3)));

            l = _mm_loadu_ps(&m30_);
            t0 = _mm_mul_ps(_mm_shuffle_ps(l, l, _MM_SHUFFLE(0, 0, 0, 0)), r0);
            t1 = _mm_mul_ps(_mm_shuffle_ps(l, l, _MM_SHUFFLE(1, 1, 1, 1)), r1);
            t2 = _mm_mul_ps(_mm_shuffle_ps(l, l, _MM_SHUFFLE(2, 2, 2, 2)), r2);
            t3 = _mm_mul_ps(_mm_shuffle_ps(l, l, _MM_SHUFFLE(3, 3, 3, 3)), r3);
            _mm_storeu_ps(&out.m30_, _mm_add_ps(_mm_add_ps(t0, t1), _mm_add_ps(t2, t3)));

            return out;
#else
            return Matrix4(
                m00_ * rhs.m00_ + m01_ * rhs.m10_ + m02_ * rhs.m20_ + m03_ * rhs.m30_,
                m00_ * rhs.m01_ + m01_ * rhs.m11_ + m02_ * rhs.m21_ + m03_ * rhs.m31_,
                m00_ * rhs.m02_ + m01_ * rhs.m12_ + m02_ * rhs.m22_ + m03_ * rhs.m32_,
                m00_ * rhs.m03_ + m01_ * rhs.m13_ + m02_ * rhs.m23_ + m03_ * rhs.m33_,
                m10_ * rhs.m00_ + m11_ * rhs.m10_ + m12_ * rhs.m20_ + m13_ * rhs.m30_,
                m10_ * rhs.m01_ + m11_ * rhs.m11_ + m12_ * rhs.m21_ + m13_ * rhs.m31_,
                m10_ * rhs.m02_ + m11_ * rhs.m12_ + m12_ * rhs.m22_ + m13_ * rhs.m32_,
                m10_ * rhs.m03_ + m11_ * rhs.m13_ + m12_ * rhs.m23_ + m13_ * rhs.m33_,
                m20_ * rhs.m00_ + m21_ * rhs.m10_ + m22_ * rhs.m20_ + m23_ * rhs.m30_,
                m20_ * rhs.m01_ + m21_ * rhs.m11_ + m22_ * rhs.m21_ + m23_ * rhs.m31_,
                m20_ * rhs.m02_ + m21_ * rhs.m12_ + m22_ * rhs.m22_ + m23_ * rhs.m32_,
                m20_ * rhs.m03_ + m21_ * rhs.m13_ + m22_ * rhs.m23_ + m23_ * rhs.m33_,
                m30_ * rhs.m00_ + m31_ * rhs.m10_ + m32_ * rhs.m20_ + m33_ * rhs.m30_,
                m30_ * rhs.m01_ + m31_ * rhs.m11_ + m32_ * rhs.m21_ + m33_ * rhs.m31_,
                m30_ * rhs.m02_ + m31_ * rhs.m12_ + m32_ * rhs.m22_ + m33_ * rhs.m32_,
                m30_ * rhs.m03_ + m31_ * rhs.m13_ + m32_ * rhs.m23_ + m33_ * rhs.m33_);
#endif
        }

        /// Set translation elements.
        void SetTranslation(const Vector3& translation)
        {
            m03_ = translation.x;
            m13_ = translation.y;
            m23_ = translation.z;
        }

        /// Set rotation elements from a 3x3 matrix.
        void SetRotation(const Matrix3& rotation)
        {
            m00_ = rotation.m00_;
            m01_ = rotation.m01_;
            m02_ = rotation.m02_;
            m10_ = rotation.m10_;
            m11_ = rotation.m11_;
            m12_ = rotation.m12_;
            m20_ = rotation.m20_;
            m21_ = rotation.m21_;
            m22_ = rotation.m22_;
        }

        /// Set scaling elements.
        void SetScale(const Vector3& scale)
        {
            m00_ = scale.x;
            m11_ = scale.y;
            m22_ = scale.z;
        }

        /// Set uniform scaling elements.
        void SetScale(float scale)
        {
            m00_ = scale;
            m11_ = scale;
            m22_ = scale;
        }

        /// Return the combined rotation and scaling matrix.
        Lumos::Maths::Matrix3 ToMatrix3() const
        {
            return Lumos::Maths::Matrix3(
                m00_,
                m01_,
                m02_,
                m10_,
                m11_,
                m12_,
                m20_,
                m21_,
                m22_);
        }

        /// Return the rotation matrix with scaling removed.
        Matrix3 RotationMatrix() const
        {
            Vector3 invScale(
                1.0f / sqrtf(m00_ * m00_ + m10_ * m10_ + m20_ * m20_),
                1.0f / sqrtf(m01_ * m01_ + m11_ * m11_ + m21_ * m21_),
                1.0f / sqrtf(m02_ * m02_ + m12_ * m12_ + m22_ * m22_));

            return ToMatrix3().Scaled(invScale);
        }

        /// Return the translation part.
        Vector3 Translation() const
        {
            return Vector3(
                m03_,
                m13_,
                m23_);
        }

        /// Return the rotation part.
        Quaternion Rotation() const { return Quaternion(RotationMatrix()); }

        /// Return the scaling part.
        Vector3 Scale() const
        {
            return Vector3(
                sqrtf(m00_ * m00_ + m10_ * m10_ + m20_ * m20_),
                sqrtf(m01_ * m01_ + m11_ * m11_ + m21_ * m21_),
                sqrtf(m02_ * m02_ + m12_ * m12_ + m22_ * m22_));
        }

        /// Return the scaling part with the sign. Reference rotation matrix is required to avoid ambiguity.
        Vector3 SignedScale(const Matrix3& rotation) const
        {
            return Vector3(
                rotation.m00_ * m00_ + rotation.m10_ * m10_ + rotation.m20_ * m20_,
                rotation.m01_ * m01_ + rotation.m11_ * m11_ + rotation.m21_ * m21_,
                rotation.m02_ * m02_ + rotation.m12_ * m12_ + rotation.m22_ * m22_);
        }

        /// Return transposed.
        Matrix4 Transpose() const
        {
#ifdef LUMOS_SSE
            __m128 m0 = _mm_loadu_ps(&m00_);
            __m128 m1 = _mm_loadu_ps(&m10_);
            __m128 m2 = _mm_loadu_ps(&m20_);
            __m128 m3 = _mm_loadu_ps(&m30_);
            _MM_TRANSPOSE4_PS(m0, m1, m2, m3); // NOLINT(modernize-use-bool-literals)
            Matrix4 out;
            _mm_storeu_ps(&out.m00_, m0);
            _mm_storeu_ps(&out.m10_, m1);
            _mm_storeu_ps(&out.m20_, m2);
            _mm_storeu_ps(&out.m30_, m3);
            return out;
#else
            return Matrix4(
                m00_,
                m10_,
                m20_,
                m30_,
                m01_,
                m11_,
                m21_,
                m31_,
                m02_,
                m12_,
                m22_,
                m32_,
                m03_,
                m13_,
                m23_,
                m33_);
#endif
        }

        /// Test for equality with another matrix with epsilon.
        bool Equals(const Matrix4& rhs, float eps = M_EPSILON) const
        {
            const float* leftData = Data();
            const float* rightData = rhs.Data();

            for(unsigned i = 0; i < 16; ++i)
            {
                if(!Lumos::Maths::Equals(leftData[i], rightData[i], eps))
                    return false;
            }

            return true;
        }

        /// Return decomposition to translation, rotation and scale.
        void Decompose(Vector3& translation, Quaternion& rotation, Vector3& scale) const;

        /// Return inverse.
        Matrix4 Inverse() const;

        /// Return float data.
        const float* Data() const { return &m00_; }

        /// Return matrix element.
        float Element(unsigned i, unsigned j) const { return Data()[i * 4 + j]; }

        float& ElementRef(unsigned i, unsigned j) { return (&m00_)[i * 4 + j]; }

        /// Return matrix row.
        Vector4 Row(unsigned i) const { return Vector4(Element(i, 0), Element(i, 1), Element(i, 2), Element(i, 3)); }

        /// Return matrix column.
        Vector4 Column(unsigned j) const { return Vector4(Element(0, j), Element(1, j), Element(2, j), Element(3, j)); }

        static Matrix4 Scale(const Vector3& scale)
        {
            Matrix4 s;
            s.SetScale(scale);
            return s;
        }

        static Matrix4 Translation(const Vector3& pos)
        {
            Matrix4 s;
            s.SetTranslation(pos);
            return s;
        }

        static Matrix4 Rotation(const Vector3& rot)
        {
            Matrix4 s;
            s.SetRotation(Quaternion::EulerAnglesToQuaternion(rot.x, rot.y, rot.z).RotationMatrix());
            return s;
        }

        static Matrix4 Perspective(float znear, float zfar, float aspect, float fov, float offsetX = 0.0f, float offsetY = 0.0f, float zoom = 1.0f);
        static Matrix4 Orthographic(float left, float right, float bottom, float top, float znear, float zfar, float offsetX = 0.0f, float offsetY = 0.0f, float zoom = 1.0f);

        /// Return whether is NaN.
        bool IsNaN() const
        {
            return (
                Lumos::Maths::IsNaN(m00_) || Lumos::Maths::IsNaN(m01_) || Lumos::Maths::IsNaN(m02_) || Lumos::Maths::IsNaN(m03_) ||

                Lumos::Maths::IsNaN(m10_) || Lumos::Maths::IsNaN(m11_) || Lumos::Maths::IsNaN(m12_) || Lumos::Maths::IsNaN(m13_) ||

                Lumos::Maths::IsNaN(m20_) || Lumos::Maths::IsNaN(m21_) || Lumos::Maths::IsNaN(m22_) || Lumos::Maths::IsNaN(m23_) ||

                Lumos::Maths::IsNaN(m30_) || Lumos::Maths::IsNaN(m31_) || Lumos::Maths::IsNaN(m32_) || Lumos::Maths::IsNaN(m33_));
        }

        /// Return whether is Inf.
        bool IsInf() const
        {
            return (
                Lumos::Maths::IsInf(m00_) || Lumos::Maths::IsInf(m01_) || Lumos::Maths::IsInf(m02_) || Lumos::Maths::IsInf(m03_) ||

                Lumos::Maths::IsInf(m10_) || Lumos::Maths::IsInf(m11_) || Lumos::Maths::IsInf(m12_) || Lumos::Maths::IsInf(m13_) ||

                Lumos::Maths::IsInf(m20_) || Lumos::Maths::IsInf(m21_) || Lumos::Maths::IsInf(m22_) || Lumos::Maths::IsInf(m23_) ||

                Lumos::Maths::IsInf(m30_) || Lumos::Maths::IsInf(m31_) || Lumos::Maths::IsInf(m32_) || Lumos::Maths::IsInf(m33_));
        }

        /// Return hash value for HashSet & HashMap.
        unsigned ToHash() const
        {
            unsigned hash = 37;
            for(int i = 0; i < 4 * 4; i++)
                hash = 37 * hash + FloatToRawIntBits(Data()[i]);
            return hash;
        }

        friend std::ostream& operator<<(std::ostream& o, const Matrix4& m)
        {
            return o << "Mat4("
                     << "/n"
                     << "\t" << m.m00_ << ", " << m.m10_ << ", " << m.m20_ << ", " << m.m30_ << ", "
                     << "/n"
                     << "\t" << m.m01_ << ", " << m.m11_ << ", " << m.m21_ << ", " << m.m31_ << ", "
                     << "/n"
                     << "\t" << m.m02_ << ", " << m.m12_ << ", " << m.m22_ << ", " << m.m32_ << ", "
                     << "/n"
                     << "\t" << m.m03_ << ", " << m.m13_ << ", " << m.m23_ << ", " << m.m33_ << "/n"
                     << " )";
        }

        float m00_ = 0.0f;
        float m01_ = 0.0f;
        float m02_ = 0.0f;
        float m03_ = 0.0f;
        float m10_ = 0.0f;
        float m11_ = 0.0f;
        float m12_ = 0.0f;
        float m13_ = 0.0f;
        float m20_ = 0.0f;
        float m21_ = 0.0f;
        float m22_ = 0.0f;
        float m23_ = 0.0f;
        float m30_ = 0.0f;
        float m31_ = 0.0f;
        float m32_ = 0.0f;
        float m33_ = 0.0f;

        /// Bulk transpose matrices.
        static void BulkTranspose(float* dest, const float* src, unsigned count)
        {
            for(unsigned i = 0; i < count; ++i)
            {
#ifdef LUMOS_SSE
                __m128 m0 = _mm_loadu_ps(src);
                __m128 m1 = _mm_loadu_ps(src + 4);
                __m128 m2 = _mm_loadu_ps(src + 8);
                __m128 m3 = _mm_loadu_ps(src + 12);
                _MM_TRANSPOSE4_PS(m0, m1, m2, m3); // NOLINT(modernize-use-bool-literals)
                _mm_storeu_ps(dest, m0);
                _mm_storeu_ps(dest + 4, m1);
                _mm_storeu_ps(dest + 8, m2);
                _mm_storeu_ps(dest + 12, m3);
#else
                dest[0] = src[0];
                dest[1] = src[4];
                dest[2] = src[8];
                dest[3] = src[12];
                dest[4] = src[1];
                dest[5] = src[5];
                dest[6] = src[9];
                dest[7] = src[13];
                dest[8] = src[2];
                dest[9] = src[6];
                dest[10] = src[10];
                dest[11] = src[14];
                dest[12] = src[3];
                dest[13] = src[7];
                dest[14] = src[11];
                dest[15] = src[15];
#endif
                dest += 16;
                src += 16;
            }
        }

        /// Zero matrix.
        static const Matrix4 ZERO;
        /// Identity matrix.
        static const Matrix4 IDENTITY;

        static Matrix4 Transpose(const Matrix4& m)
        {
            return m.Transpose();
        }

        static Matrix4 Inverse(const Matrix4& m)
        {
            return m.Inverse();
        }
#ifdef LUMOS_SSE
    private:
        /// \brief Sets this matrix from the given translation, rotation (as quaternion (w,x,y,z)), and nonuniform scale (x,y,z) parameters. Note: the w component of the scale parameter passed to this function must be 1.
        void inline SetFromTRS(__m128 t, __m128 q, __m128 s)
        {
            q = _mm_shuffle_ps(q, q, _MM_SHUFFLE(0, 3, 2, 1));
            __m128 one = _mm_set_ps(0, 0, 0, 1);
            const __m128 sseX1 = _mm_castsi128_ps(_mm_set_epi32((int)0x80000000UL, (int)0x80000000UL, 0, (int)0x80000000UL));
            __m128 q2 = _mm_add_ps(q, q);
            __m128 t2 = _mm_add_ss(_mm_xor_ps(_mm_mul_ps(_mm_shuffle_ps(q, q, _MM_SHUFFLE(3, 3, 3, 2)), _mm_shuffle_ps(q2, q2, _MM_SHUFFLE(0, 1, 2, 2))), sseX1), one);
            const __m128 sseX0 = _mm_shuffle_ps(sseX1, sseX1, _MM_SHUFFLE(0, 3, 2, 1));
            __m128 t0 = _mm_mul_ps(_mm_shuffle_ps(q, q, _MM_SHUFFLE(1, 0, 0, 1)), _mm_shuffle_ps(q2, q2, _MM_SHUFFLE(2, 2, 1, 1)));
            __m128 t1 = _mm_xor_ps(t0, sseX0);
            __m128 r0 = _mm_sub_ps(t2, t1);
            __m128 xx2 = _mm_mul_ss(q, q2);
            __m128 r1 = _mm_sub_ps(_mm_xor_ps(t2, sseX0), _mm_move_ss(t1, xx2));
            r1 = _mm_shuffle_ps(r1, r1, _MM_SHUFFLE(2, 3, 0, 1));
            __m128 r2 = _mm_shuffle_ps(_mm_movehl_ps(r0, r1), _mm_sub_ss(_mm_sub_ss(one, xx2), t0), _MM_SHUFFLE(2, 0, 3, 1));
            __m128 tmp0 = _mm_unpacklo_ps(r0, r1);
            __m128 tmp2 = _mm_unpacklo_ps(r2, t);
            __m128 tmp1 = _mm_unpackhi_ps(r0, r1);
            __m128 tmp3 = _mm_unpackhi_ps(r2, t);
            _mm_storeu_ps(&m00_, _mm_mul_ps(_mm_movelh_ps(tmp0, tmp2), s));
            _mm_storeu_ps(&m10_, _mm_mul_ps(_mm_movehl_ps(tmp2, tmp0), s));
            _mm_storeu_ps(&m20_, _mm_mul_ps(_mm_movelh_ps(tmp1, tmp3), s));
        }
#endif

    };

    /// Multiply a 4x4 matrix with a scalar.
    inline Matrix4 operator*(float lhs, const Matrix4& rhs) { return rhs * lhs; }

}
