#pragma once
#ifdef LUMOS_SSE
#include <smmintrin.h>
#endif

#define CLIP_CONTROL_ZO_BIT (1 << 0) // ZERO_TO_ONE
#define CLIP_CONTROL_NO_BIT (1 << 1) // NEGATIVE_ONE_TO_ONE
#define CLIP_CONTROL_LH_BIT (1 << 2) // LEFT_HANDED, For DirectX, Metal, Vulkan
#define CLIP_CONTROL_RH_BIT (1 << 3) // RIGHT_HANDED, For OpenGL, default

#define CLIP_CONTROL_LH_ZO (CLIP_CONTROL_LH_BIT | CLIP_CONTROL_ZO_BIT)
#define CLIP_CONTROL_LH_NO (CLIP_CONTROL_LH_BIT | CLIP_CONTROL_NO_BIT)
#define CLIP_CONTROL_RH_ZO (CLIP_CONTROL_RH_BIT | CLIP_CONTROL_ZO_BIT)
#define CLIP_CONTROL_RH_NO (CLIP_CONTROL_RH_BIT | CLIP_CONTROL_NO_BIT)

namespace Lumos
{
    namespace Maths
    {
        class Vector3;
        class Vector4;
        class Matrix3;
        class Quaternion;

        class LUMOS_EXPORT MEM_ALIGN Matrix4
        {
        public:
            static int CONFIG_CLIP_CONTROL;
            static void SetUpCoordSystem(bool LeftHanded, bool forceZeroToOne);

            Matrix4()
            {
                ToIdentity();
            }

            Matrix4(float value)
            {
                ToZero();
                values[0]  = value;
                values[5]  = value;
                values[10] = value;
                values[15] = value;
            }

            Matrix4(const float elements[16]);

            Matrix4(float v00, float v01, float v02, float v03,
                    float v10, float v11, float v12, float v13,
                    float v20, float v21, float v22, float v23,
                    float v30, float v31, float v32, float v33) noexcept
            {
                values[0]  = v00;
                values[1]  = v01;
                values[2]  = v02;
                values[3]  = v03;
                values[4]  = v10;
                values[5]  = v11;
                values[6]  = v12;
                values[7]  = v13;
                values[8]  = v20;
                values[9]  = v21;
                values[10] = v22;
                values[11] = v23;
                values[12] = v30;
                values[13] = v31;
                values[14] = v32;
                values[15] = v33;
            }

            Matrix4(const Matrix3& mat);
            Matrix4(const Matrix4& mat);
            explicit Matrix4(const Quaternion& quat);
#ifdef LUMOS_SSE
            struct
            {
                union
                {
                    float values[16];
                    __m128 mmvalues[4];
                };
            } MEM_ALIGN;
#else
            float values[16];
#endif
            void ToZero();
            void ToIdentity();

            inline float Get(unsigned int row, unsigned int column) const
            {
                return values[row + column * 4];
            }

            inline float& Get(unsigned int row, unsigned int column)
            {
                return values[row + column * 4];
            }

            Vector4 GetRow(unsigned int row) const;
            Vector4 GetCol(unsigned int column) const;

            void SetRow(unsigned int row, const Vector4& val);
            void SetCol(unsigned int column, const Vector4& val);
            Vector3 GetPositionVector() const;

#if defined(LUMOS_SSE) && defined(LUMOS_SSE)
            Vector3 GetTranslationMemAligned() const;
#endif
            void SetPositionVector(const Vector3& v);
            Vector3 GetScaling() const;
            void SetScaling(const Vector3& in);

            Matrix4 Inverse() const
            {
                return Inverse(*this);
            }

            void SetTranslation(const Vector3& translation);

            Vector3 Translation() const;
            Quaternion Rotation() const;
            Vector3 Scale() const;

            void Decompose(Vector3& translation, Quaternion& rotation, Vector3& scale) const;

            Matrix4 operator*(const Matrix4& m) const;
            Vector3 operator*(const Vector3& v) const;
            Vector4 operator*(const Vector4& v) const;
            Matrix4 operator*(float rhs) const;

            bool operator==(const Matrix4& m) const;

            Matrix4 Transpose() const;
            Matrix4 GetRotation() const;

            Quaternion ToQuaternion() const;

            static Vector3 GetEulerAngles(const Matrix4& mat);
            static Matrix4 RotationX(float degrees);
            static Matrix4 RotationY(float degrees);
            static Matrix4 RotationZ(float degrees);
            static Matrix4 Rotation(float degrees, const Vector3& axis);
            static Matrix4 Rotation(float degreesX, float degreesY, float degreesZ);
            static Matrix4 Scale(const Vector3& scale);
            static Matrix4 Translation(const Vector3& translation);
            static Matrix4 Inverse(const Matrix4& inM);
            static Matrix4 Perspective(float znear, float zfar, float aspect, float fov);
            static Matrix4 Orthographic(float left, float right, float bottom, float top, float znear, float zfar);
            static Matrix4 LookAt(const Vector3& from, const Vector3& lookingAt,
                                  const Vector3& up);

            inline float operator[](const int index) const
            {
                return values[index];
            }

            inline float& operator[](const int index)
            {
                return values[index];
            }
        };

        inline Matrix4 operator*(float lhs, const Matrix4& rhs)
        {
            return rhs * lhs;
        }
    }

    typedef Maths::Matrix4 Mat4;
}
