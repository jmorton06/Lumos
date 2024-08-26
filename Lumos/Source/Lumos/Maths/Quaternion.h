#pragma once
#include "Vector3.h"
namespace Lumos
{
    namespace Maths
    {
        class Vector3;
        class Matrix3;
        class Matrix4;

        class LUMOS_EXPORT MEM_ALIGN Quaternion
        {
        public:
            Quaternion();
            Quaternion(float test)
            {
                x = test;
                y = test;
                z = test;
                w = test;
            }
            Quaternion(const Vector3& vec, float w);
            Quaternion(float lx, float ly, float lz, float lw);
            Quaternion(const Quaternion& v);

            // Construct from Euler (Degrees)
            Quaternion(float pitch, float yaw, float roll);
            // Construct from Euler (Degrees)
            Quaternion(const Vector3& v);

#ifdef LUMOS_SSE
            Quaternion(__m128 m);
#endif

#ifdef LUMOS_SSE
            union
            {
                struct
                {
                    float x;
                    float y;
                    float z;
                    float w;
                };
                __m128 mmvalue;
            } MEM_ALIGN;
#else
            float x;
            float y;
            float z;
            float w;
#endif

            void Normalise();
            Quaternion Normalised() const;

            Matrix4 ToMatrix4() const;
            Matrix3 ToMatrix3() const;

            Quaternion Inverse() const;
            Quaternion Conjugate() const;
            float Magnitude() const;
            float LengthSquared() const;

            void GenerateW(); // builds 4th component when loading in shortened, 3 component quaternions
            Vector3 ToEuler() const;
            void FromEulerAngles(float pitch, float yaw, float roll);

            bool IsValid() const;
            bool IsInf() const;
            bool IsNaN() const;

            static Quaternion EulerAnglesToQuaternion(float pitch, float yaw, float roll);
            static Quaternion AxisAngleToQuaterion(const Vector3& vector, float degrees);

            static void RotatePointByQuaternion(const Quaternion& quat, Vector3& point);

            static Quaternion FromMatrix(const Matrix4& matrix);
            static Quaternion FromVectors(const Vector3& v1, const Vector3& v2);

            static Quaternion LookAt(const Vector3& from, const Vector3& to, const Vector3& up = Vector3(0, 1, 0));
            static Quaternion GetRotation(const Vector3& from_dir, const Vector3& to_dir, const Vector3& up = Vector3(0, 1, 0));
            static Quaternion Rotation(const Vector3& unitVec0, const Vector3& unitVec1);
            static Quaternion Rotation(float radians, const Vector3& unitVec);

            static Quaternion RotationX(float radians);
            static Quaternion RotationY(float radians);
            static Quaternion RotationZ(float radians);

            Vector3 Transform(const Vector3& point) const;

            float Dot(const Quaternion& q) const;
            static float Dot(const Quaternion& a, const Quaternion& b);
            static Quaternion Lerp(const Quaternion& pStart, const Quaternion& pEnd, float pFactor);
            static Quaternion Slerp(const Quaternion& pStart, const Quaternion& pEnd, float pFactor);
            Quaternion Interpolate(const Quaternion& pStart, const Quaternion& pEnd, float pFactor) const;

            Quaternion operator*(const Quaternion& q) const;
            Quaternion operator+(const Quaternion& a) const;
            Quaternion operator*(float rhs) const;
            Quaternion operator-() const;
            Quaternion operator-(const Quaternion& rhs) const;
            Quaternion& operator=(const Quaternion& rhs) noexcept;
            Quaternion& operator+=(const Quaternion& rhs);
            Quaternion& operator*=(float rhs);

            Vector3 operator*(const Vector3& rhs) const;
            bool operator==(const Quaternion& rhs) const;
            bool operator!=(const Quaternion& rhs) const;
            bool operator<=(const Quaternion& rhs) const;
            bool operator>=(const Quaternion& rhs) const;
        };
    }

    typedef Maths::Quaternion Quat;
}
