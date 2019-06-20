#pragma once

#include "LM.h"
#include "Vector3.h"
#include "MathsCommon.h"

namespace Lumos
{
	namespace Maths
	{

		class Matrix4;

		class LUMOS_EXPORT MEM_ALIGN Matrix3
		{
		public:
			Matrix3()
			{
				ToIdentity();
			}

			Matrix3(const float(&elements)[9])
			{
				memcpy(values, elements, 9 * sizeof(float));
			}

			Matrix3(const Vector3 &v1, const Vector3 &v2, const Vector3 &v3);

			Matrix3(
				float a1, float a2, float a3,
				float b1, float b2, float b3,
				float c1, float c2, float c3);

			Matrix3(const Matrix4 &m4);

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

			//Set all matrix values to zero
			inline Matrix3 &ToZero()
			{
				memset(values, 0, 9 * sizeof(float));
				return *this;
			}

			inline Matrix3 &ToIdentity()
			{
				memcpy(values, Matrix3::IDENTITY_DATA, 9 * sizeof(float));
				return *this;
			}


			inline float operator[](int index) const
			{
				return values[index];
			}

			inline float &operator[](int index)
			{
				return values[index];
			}

			inline float operator()(int row, int col) const
			{
				return values[row + col * 3];
			}

			inline float &operator()(int row, int col)
			{
				return values[row + col * 3];
			}

			inline Vector3 GetRow(unsigned int row) const
			{
				return Vector3(values[row], values[row + 3], values[row + 6]);
			}

			inline Vector3 GetCol(unsigned int column) const
			{
				Vector3 out;
				memcpy(&out, &values[3 * column], sizeof(Vector3));
				return out;
			}

			inline void SetRow(unsigned int row, const Vector3 &val)
			{
				values[row] = val.GetX();
				values[row + 3] = val.GetY();
				values[row + 6] = val.GetZ();
			}

			inline void SetCol(unsigned int column, const Vector3 &val)
			{
				memcpy(&values[column * 3], &val, sizeof(Vector3));
			}

			inline Vector3 GetDiagonal() const
			{
				return Vector3(values[0], values[4], values[8]);
			}

			inline void SetDiagonal(const Vector3 &v)
			{
				values[0] = v.GetX();
				values[4] = v.GetY();
				values[8] = v.GetZ();
			}

			inline Vector3 GetScalingVector() const { return Vector3(_11, _22, _33); }
			inline void	SetScalingVector(const Vector3& in)
            {
                _11 = in.GetX();
                _22 = in.GetY();
                _33 = in.GetZ();
            }

			inline void Transpose()
			{
#ifdef LUMOS_SSEMAT3
				__m128 empty = _mm_setzero_ps();
				__m128 column1 = _mm_loadu_ps(&values[0]);
				__m128 column2 = _mm_loadu_ps(&values[3]);
				__m128 column3 = _mm_loadu_ps(&values[6]);

				_MM_TRANSPOSE4_PS(column1, column2, column3, empty);

				_mm_storeu_ps(&values[0], column1);
				_mm_storeu_ps(&values[3], column2);
				values[6] = GetValue(column3, 0);
				values[7] = GetValue(column3, 1);
				values[8] = GetValue(column3, 2);
#else
				Matrix3 m;
				m._11 = _11;
				m._21 = _12;
				m._31 = _13;

				m._12 = _21;
				m._22 = _22;
				m._32 = _23;

				m._13 = _31;
				m._23 = _32;
				m._33 = _33;

				*this = m;
#endif
			}

			// Standard Matrix Functionality
			static Matrix3 Inverse(const Matrix3 &rhs);
			static Matrix3 Transpose(const Matrix3 &rhs);
			static Matrix3 Adjugate(const Matrix3 &m);
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

			//Creates a rotation matrix that rotates by 'degrees' around the 'axis'
			static Matrix3 Rotation(float degrees, const Vector3 &axis);
			static Matrix3 Rotation(float degreesX, float degreesY, float degreesZ);

			//Creates a scaling matrix (puts the 'scale' vector down the diagonal)
			static Matrix3 Scale(const Vector3 &scale);

			friend std::ostream &operator<<(std::ostream &o, const Matrix3 &m);

			Vector3 operator*(const Vector3& v) const;
			Matrix3 operator*(const Matrix3& m) const;
		};
	}
}
