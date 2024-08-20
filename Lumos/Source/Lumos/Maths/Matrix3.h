#pragma once

#ifdef LUMOS_SSE
#include <smmintrin.h>
#endif

namespace Lumos
{
    namespace Maths
    {

        class Vector3;
        class Matrix4;
        class Quaternion;

        class LUMOS_EXPORT MEM_ALIGN Matrix3
        {
        public:
            Matrix3()
            {
                ToIdentity();
            }

            Matrix3(float x)
            {
                ToIdentity();

                values[0] = x;
                values[4] = x;
                values[8] = x;
            }

            Matrix3(const float (&elements)[9]);
            Matrix3(const Vector3& v1, const Vector3& v2, const Vector3& v3);

            Matrix3(
                float a1, float a2, float a3,
                float b1, float b2, float b3,
                float c1, float c2, float c3);

            Matrix3(const Matrix4& m4);
            explicit Matrix3(const Quaternion& q);

            union
            {
                float values[9];
                struct
                {
                    float _11, _12, _13;
                    float _21, _22, _23;
                    float _31, _32, _33;
                };
            } MEM_ALIGN;

            // Set all matrix values to zero
            Matrix3& ToZero();
            Matrix3& ToIdentity();

            inline float operator[](const int index) const
            {
                return values[index];
            }

            inline float& operator[](const int index)
            {
                return values[index];
            }

            inline float operator()(int row, int col) const
            {
                return values[row + col * 3];
            }

            inline float& operator()(int row, int col)
            {
                return values[row + col * 3];
            }

            Vector3 GetRow(unsigned int row) const;
            Vector3 GetCol(unsigned int column) const;
            void SetRow(unsigned int row, const Vector3& val);
            void SetCol(unsigned int column, const Vector3& val);

            void SetDiagonal(const Vector3& v);
            Vector3 GetDiagonal() const;
            Vector3 GetScalingVector() const;
            void SetScalingVector(const Vector3& in);

            void Transpose();

            // Standard Matrix Functionality
            static Matrix3 Inverse(const Matrix3& rhs);
            static Matrix3 Transpose(const Matrix3& rhs);
            static Matrix3 Adjugate(const Matrix3& m);
            static Matrix3 OuterProduct(const Vector3& a, const Vector3& b);
            // Additional Functionality
            float Trace() const;
            float Determinant() const;
            Matrix3 Inverse() const;

            bool TryInvert();
            bool TryTransposedInvert();

            static const float EMPTY_DATA[9];
            static const float IDENTITY_DATA[9];

            static const Matrix3 EMPTY;
            static const Matrix3 IDENTITY;
            static const Matrix3 ZeroMatrix;

            static Matrix3 RotationX(float degrees);
            static Matrix3 RotationY(float degrees);
            static Matrix3 RotationZ(float degrees);

            // Creates a rotation matrix that rotates by 'degrees' around the 'axis'
            static Matrix3 Rotation(float degrees, const Vector3& axis);
            static Matrix3 Rotation(float degreesX, float degreesY, float degreesZ);

            // Creates a scaling matrix (puts the 'scale' vector down the diagonal)
            static Matrix3 Scale(const Vector3& scale);

            Vector3 operator*(const Vector3& v) const;
            Matrix3 operator*(const Matrix3& m) const;
        };
    }

    typedef Maths::Matrix3 Mat3;
}
