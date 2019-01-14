#pragma once
#include "LM.h"
#include "Vector3.h"
#include "Vector4.h"
#include "MathsCommon.h"

namespace Lumos
{
	namespace maths
	{
		class Vector3;
		class Matrix3;

#ifdef LUMOS_SSEMAT4

		class LUMOS_EXPORT MEM_ALIGN Matrix4
		{
		public:
			inline Matrix4()
			{
				ToIdentity();
			}

			inline Matrix4(const float elements[16])
			{
				memcpy(values, elements, 16 * sizeof(float));
			}

			Matrix4(const Matrix3 &mat);

			Matrix4(const Matrix4 &mat);

			struct 
            {
				union 
                {
					float values[16];
					__m128 mmvalues[4];
				};
			} MEM_ALIGN;

			inline void ToZero()
			{
				mmvalues[0] = mmvalues[1] = mmvalues[2] = mmvalues[3] = _mm_setzero_ps();
			}

			inline void ToIdentity()
			{
				mmvalues[0] = _mm_set_ps(0, 0, 0, 1);
				mmvalues[1] = _mm_set_ps(0, 0, 1, 0);
				mmvalues[2] = _mm_set_ps(0, 1, 0, 0);
				mmvalues[3] = _mm_set_ps(1, 0, 0, 0);
			}

			inline Vector4 GetRow(unsigned int row) const
			{
				return Vector4(values[row], values[row + 4], values[row + 8], values[row + 12]);
			}

			inline Vector4 GetCol(unsigned int column) const
			{
#ifdef LUMOS_SSEVEC4
				return Vector4(mmvalues[column]);
#else
				return Vector4();// mmvalues[column]);
#endif
			}

			inline void SetRow(unsigned int row, const Vector4 &val)
			{
				values[row] = val.GetX();
				values[row + 4] = val.GetY();
				values[row + 8] = val.GetZ();
				values[row + 12] = val.GetW();
			}

			inline void SetCol(unsigned int column, const Vector4 &val) {
#ifdef LUMOS_SSEVEC4
				mmvalues[column] = val.m_Value;
#endif
			}

			inline Vector3 GetPositionVector() const
			{
				return Vector3(values[12], values[13], values[14]);
			}

#ifdef LUMOS_SSEVEC3

			inline Vector3 GetTranslationMemAligned() const
			{
				return Vector3(mmvalues[3]);
			}

#endif

			inline void SetPositionVector(const Vector3 &v)
			{
				values[12] = v.GetX();
				values[13] = v.GetY();
				values[14] = v.GetZ();
			}

			inline Vector3 GetScaling() const { return Vector3(values[0], values[5], values[10]); }

			inline Vector3 GetScalingMemAligned() const { return Vector3(values[0], values[5], values[10]); }

			inline void SetScaling(const Vector3 &in)
			{
				values[0] = in.GetX();
				values[5] = in.GetY();
				values[10] = in.GetZ();
			}

			Matrix4 GetRotation() const;

			Matrix4 operator*(const Matrix4 &m) const;

			Vector3 operator*(const Vector3 &v) const;

			Vector4 operator*(const Vector4 &v) const;

			void Transpose();

			static Vector3 GetEulerAngles(const Matrix4 &mat);

			static Matrix4 RotationX(float degrees);

			static Matrix4 RotationY(float degrees);

			static Matrix4 RotationZ(float degrees);

			static Matrix4 Rotation(float degrees, const Vector3 &axis);

			static Matrix4 Rotation(float degreesX, float degreesY, float degreesZ);

			static Matrix4 Scale(const Vector3 &scale);

			static Matrix4 Translation(const Vector3 &translation);

			static Matrix4 Perspective(float znear, float zfar, float aspect, float fov);

			static Matrix4 Orthographic(float znear, float zfar, float right, float left, float top, float bottom);

			static Matrix4 BuildViewMatrix(const Vector3 &from, const Vector3 &lookingAt,
										   const Vector3 &up = Vector3(0.0f, 1.0f, 0.0f));

			MEM_ALIGN_NEW_DELETE

			friend std::ostream &operator<<(std::ostream &o, const Matrix4 &m);

			inline float operator[](const int index) const {
				return values[index];
			}

			inline float &operator[](const int index) {
				return values[index];
			}

			//https://lxjk.github.io/2017/09/03/Fast-4x4-Matrix-Inverse-with-SSE-SIMD-Explained.html

#define MakeShuffleMask(x, y, z, w)           (x | (y<<2) | (z<<4) | (w<<6))

			// vec(0, 1, 2, 3) -> (vec[x], vec[y], vec[z], vec[w])
#define VecSwizzle(vec, x, y, z, w)           _mm_shuffle_ps(vec, vec, MakeShuffleMask(x,y,z,w))
#define VecSwizzle1(vec, x)                _mm_shuffle_ps(vec, vec, MakeShuffleMask(x,x,x,x))
			// special swizzle
#define VecSwizzle_0101(vec)               _mm_movelh_ps(vec, vec)
#define VecSwizzle_2323(vec)               _mm_movehl_ps(vec, vec)
#define VecSwizzle_0022(vec)               _mm_moveldup_ps(vec)
#define VecSwizzle_1133(vec)               _mm_movehdup_ps(vec)

			// return (vec1[x], vec1[y], vec2[z], vec2[w])
#define VecShuffle(vec1, vec2, x, y, z, w)    _mm_shuffle_ps(vec1, vec2, MakeShuffleMask(x,y,z,w))
			// special shuffle
#define VecShuffle_0101(vec1, vec2)        _mm_movelh_ps(vec1, vec2)
#define VecShuffle_2323(vec1, vec2)        _mm_movehl_ps(vec2, vec1)

			// for row major matrix
			// we use __m128 to represent 2x2 matrix as A = | A0  A1 |
			//                                              | A2  A3 |
			// 2x2 row major Matrix multiply A*B
			static __m128 Mat2Mul(__m128 vec1, __m128 vec2) {
				return
						_mm_add_ps(_mm_mul_ps(vec1, VecSwizzle(vec2, 0, 3, 0, 3)),
								   _mm_mul_ps(VecSwizzle(vec1, 1, 0, 3, 2), VecSwizzle(vec2, 2, 1, 2, 1)));
			}

			// 2x2 row major Matrix adjugate multiply (A#)*B
			static __m128 Mat2AdjMul(__m128 vec1, __m128 vec2) {
				return
						_mm_sub_ps(_mm_mul_ps(VecSwizzle(vec1, 3, 3, 0, 0), vec2),
								   _mm_mul_ps(VecSwizzle(vec1, 1, 1, 2, 2), VecSwizzle(vec2, 2, 3, 0, 1)));

			}

			// 2x2 row major Matrix multiply adjugate A*(B#)
			static __m128 Mat2MulAdj(__m128 vec1, __m128 vec2) {
				return
						_mm_sub_ps(_mm_mul_ps(vec1, VecSwizzle(vec2, 3, 0, 3, 0)),
								   _mm_mul_ps(VecSwizzle(vec1, 1, 0, 3, 2), VecSwizzle(vec2, 2, 1, 2, 1)));
			}

			static Matrix4 Inverse(const Matrix4 &inM) {
#if 1
				// use block matrix method
				// A is a matrix, then i(A) or iA means inverse of A, A# (or A_ in code) means adjugate of A, |A| (or detA in code) is determinant, tr(A) is trace

				// sub matrices
				__m128 A = VecShuffle_0101(inM.mmvalues[0], inM.mmvalues[1]);
				__m128 B = VecShuffle_2323(inM.mmvalues[0], inM.mmvalues[1]);
				__m128 C = VecShuffle_0101(inM.mmvalues[2], inM.mmvalues[3]);
				__m128 D = VecShuffle_2323(inM.mmvalues[2], inM.mmvalues[3]);

				__m128 detA = _mm_set1_ps(inM.values[0] * inM.values[5] - inM.values[1] * inM.values[4]);
				__m128 detB = _mm_set1_ps(inM.values[2] * inM.values[7] - inM.values[3] * inM.values[6]);
				__m128 detC = _mm_set1_ps(inM.values[8] * inM.values[13] - inM.values[9] * inM.values[12]);
				__m128 detD = _mm_set1_ps(inM.values[10] * inM.values[15] - inM.values[11] * inM.values[14]);

#if 0 // for determinant, float version is faster
                // determinant as (|A| |B| |C| |D|)
                __m128 detSub = _mm_sub_ps(
                    _mm_mul_ps(VecShuffle(inM.mVec[0], inM.mVec[2], 0,2,0,2), VecShuffle(inM.mVec[1], inM.mVec[3], 1,3,1,3)),
                    _mm_mul_ps(VecShuffle(inM.mVec[0], inM.mVec[2], 1,3,1,3), VecShuffle(inM.mVec[1], inM.mVec[3], 0,2,0,2))
                    );
                __m128 detA = VecSwizzle1(detSub, 0);
                __m128 detB = VecSwizzle1(detSub, 1);
                __m128 detC = VecSwizzle1(detSub, 2);
                __m128 detD = VecSwizzle1(detSub, 3);
#endif

				// let iM = 1/|M| * | X  Y |
				//                  | Z  W |

				// D#C
				__m128 D_C = Mat2AdjMul(D, C);
				// A#B
				__m128 A_B = Mat2AdjMul(A, B);
				// X# = |D|A - B(D#C)
				__m128 X_ = _mm_sub_ps(_mm_mul_ps(detD, A), Mat2Mul(B, D_C));
				// W# = |A|D - C(A#B)
				__m128 W_ = _mm_sub_ps(_mm_mul_ps(detA, D), Mat2Mul(C, A_B));

				// |M| = |A|*|D| + ... (continue later)
				__m128 detM = _mm_mul_ps(detA, detD);

				// Y# = |B|C - D(A#B)#
				__m128 Y_ = _mm_sub_ps(_mm_mul_ps(detB, C), Mat2MulAdj(D, A_B));
				// Z# = |C|B - A(D#C)#
				__m128 Z_ = _mm_sub_ps(_mm_mul_ps(detC, B), Mat2MulAdj(A, D_C));

				// |M| = |A|*|D| + |B|*|C| ... (continue later)
				detM = _mm_add_ps(detM, _mm_mul_ps(detB, detC));

				// tr((A#B)(D#C))
				__m128 tr = _mm_mul_ps(A_B, VecSwizzle(D_C, 0, 2, 1, 3));
				tr = _mm_hadd_ps(tr, tr);
				tr = _mm_hadd_ps(tr, tr);
				// |M| = |A|*|D| + |B|*|C| - tr((A#B)(D#C)
				detM = _mm_sub_ps(detM, tr);

				const __m128 adjSignMask = _mm_setr_ps(1.f, -1.f, -1.f, 1.f);
				// (1/|M|, -1/|M|, -1/|M|, 1/|M|)
				__m128 rDetM = _mm_div_ps(adjSignMask, detM);

				X_ = _mm_mul_ps(X_, rDetM);
				Y_ = _mm_mul_ps(Y_, rDetM);
				Z_ = _mm_mul_ps(Z_, rDetM);
				W_ = _mm_mul_ps(W_, rDetM);

				Matrix4 r;

				// apply adjugate and store, here we combine adjugate shuffle and store shuffle
				r.mmvalues[0] = VecShuffle(X_, Y_, 3, 1, 3, 1);
				r.mmvalues[1] = VecShuffle(X_, Y_, 2, 0, 2, 0);
				r.mmvalues[2] = VecShuffle(Z_, W_, 3, 1, 3, 1);
				r.mmvalues[3] = VecShuffle(Z_, W_, 2, 0, 2, 0);

				return r;
#else
                Matrix4 inv;

                inv[0] = rhs[5] * rhs[10] * rhs[15] -
                    rhs[5] * rhs[11] * rhs[14] -
                    rhs[9] * rhs[6] * rhs[15] +
                    rhs[9] * rhs[7] * rhs[14] +
                    rhs[13] * rhs[6] * rhs[11] -
                    rhs[13] * rhs[7] * rhs[10];

                inv[4] = -rhs[4] * rhs[10] * rhs[15] +
                    rhs[4] * rhs[11] * rhs[14] +
                    rhs[8] * rhs[6] * rhs[15] -
                    rhs[8] * rhs[7] * rhs[14] -
                    rhs[12] * rhs[6] * rhs[11] +
                    rhs[12] * rhs[7] * rhs[10];

                inv[8] = rhs[4] * rhs[9] * rhs[15] -
                    rhs[4] * rhs[11] * rhs[13] -
                    rhs[8] * rhs[5] * rhs[15] +
                    rhs[8] * rhs[7] * rhs[13] +
                    rhs[12] * rhs[5] * rhs[11] -
                    rhs[12] * rhs[7] * rhs[9];

                inv[12] = -rhs[4] * rhs[9] * rhs[14] +
                    rhs[4] * rhs[10] * rhs[13] +
                    rhs[8] * rhs[5] * rhs[14] -
                    rhs[8] * rhs[6] * rhs[13] -
                    rhs[12] * rhs[5] * rhs[10] +
                    rhs[12] * rhs[6] * rhs[9];

                inv[1] = -rhs[1] * rhs[10] * rhs[15] +
                    rhs[1] * rhs[11] * rhs[14] +
                    rhs[9] * rhs[2] * rhs[15] -
                    rhs[9] * rhs[3] * rhs[14] -
                    rhs[13] * rhs[2] * rhs[11] +
                    rhs[13] * rhs[3] * rhs[10];

                inv[5] = rhs[0] * rhs[10] * rhs[15] -
                    rhs[0] * rhs[11] * rhs[14] -
                    rhs[8] * rhs[2] * rhs[15] +
                    rhs[8] * rhs[3] * rhs[14] +
                    rhs[12] * rhs[2] * rhs[11] -
                    rhs[12] * rhs[3] * rhs[10];

                inv[9] = -rhs[0] * rhs[9] * rhs[15] +
                    rhs[0] * rhs[11] * rhs[13] +
                    rhs[8] * rhs[1] * rhs[15] -
                    rhs[8] * rhs[3] * rhs[13] -
                    rhs[12] * rhs[1] * rhs[11] +
                    rhs[12] * rhs[3] * rhs[9];

                inv[13] = rhs[0] * rhs[9] * rhs[14] -
                    rhs[0] * rhs[10] * rhs[13] -
                    rhs[8] * rhs[1] * rhs[14] +
                    rhs[8] * rhs[2] * rhs[13] +
                    rhs[12] * rhs[1] * rhs[10] -
                    rhs[12] * rhs[2] * rhs[9];

                inv[2] = rhs[1] * rhs[6] * rhs[15] -
                    rhs[1] * rhs[7] * rhs[14] -
                    rhs[5] * rhs[2] * rhs[15] +
                    rhs[5] * rhs[3] * rhs[14] +
                    rhs[13] * rhs[2] * rhs[7] -
                    rhs[13] * rhs[3] * rhs[6];

                inv[6] = -rhs[0] * rhs[6] * rhs[15] +
                    rhs[0] * rhs[7] * rhs[14] +
                    rhs[4] * rhs[2] * rhs[15] -
                    rhs[4] * rhs[3] * rhs[14] -
                    rhs[12] * rhs[2] * rhs[7] +
                    rhs[12] * rhs[3] * rhs[6];

                inv[10] = rhs[0] * rhs[5] * rhs[15] -
                    rhs[0] * rhs[7] * rhs[13] -
                    rhs[4] * rhs[1] * rhs[15] +
                    rhs[4] * rhs[3] * rhs[13] +
                    rhs[12] * rhs[1] * rhs[7] -
                    rhs[12] * rhs[3] * rhs[5];

                inv[14] = -rhs[0] * rhs[5] * rhs[14] +
                    rhs[0] * rhs[6] * rhs[13] +
                    rhs[4] * rhs[1] * rhs[14] -
                    rhs[4] * rhs[2] * rhs[13] -
                    rhs[12] * rhs[1] * rhs[6] +
                    rhs[12] * rhs[2] * rhs[5];

                inv[3] = -rhs[1] * rhs[6] * rhs[11] +
                    rhs[1] * rhs[7] * rhs[10] +
                    rhs[5] * rhs[2] * rhs[11] -
                    rhs[5] * rhs[3] * rhs[10] -
                    rhs[9] * rhs[2] * rhs[7] +
                    rhs[9] * rhs[3] * rhs[6];

                inv[7] = rhs[0] * rhs[6] * rhs[11] -
                    rhs[0] * rhs[7] * rhs[10] -
                    rhs[4] * rhs[2] * rhs[11] +
                    rhs[4] * rhs[3] * rhs[10] +
                    rhs[8] * rhs[2] * rhs[7] -
                    rhs[8] * rhs[3] * rhs[6];

                inv[11] = -rhs[0] * rhs[5] * rhs[11] +
                    rhs[0] * rhs[7] * rhs[9] +
                    rhs[4] * rhs[1] * rhs[11] -
                    rhs[4] * rhs[3] * rhs[9] -
                    rhs[8] * rhs[1] * rhs[7] +
                    rhs[8] * rhs[3] * rhs[5];

                inv[15] = rhs[0] * rhs[5] * rhs[10] -
                    rhs[0] * rhs[6] * rhs[9] -
                    rhs[4] * rhs[1] * rhs[10] +
                    rhs[4] * rhs[2] * rhs[9] +
                    rhs[8] * rhs[1] * rhs[6] -
                    rhs[8] * rhs[2] * rhs[5];

                float det = rhs[0] * inv[0] + rhs[1] * inv[4] + rhs[2] * inv[8] + rhs[3] * inv[12];

                if (det == 0)
                {
                    inv.ToIdentity();
                    return inv;
                }

                det = 1.f / det;

                for (int i = 0; i < 16; i++)
                {
                    inv[i] = inv[i] * det;
                }

                return inv;
#endif
			}
		};

#else

        class LUMOS_EXPORT Matrix4
        {
        public:

            Matrix4()
            {
                ToIdentity();
            }

            explicit Matrix4(const float elements[16]);
            explicit Matrix4(const Matrix3& mat33);
            ~Matrix4();

            float	values[16];

            void	ToZero();
            void	ToIdentity();

            Vector3 GetPositionVector() const;
            void	SetPositionVector(const Vector3 in);
            Vector3 GetScalingVector() const;
            void	SetScalingVector(const Vector3 &in);

            void Transpose();

            static Matrix4 RotationX(float degrees);
            static Matrix4 RotationY(float degrees);
            static Matrix4 RotationZ(float degrees);

            static Matrix4 Rotation(float degrees, const Vector3 &axis);

            static Matrix4 Scale(const Vector3 &scale);
            static Matrix4 Translation(const Vector3 &translation);
            static Matrix4 Perspective(float znear, float zfar, float aspect, float fov);
            static Matrix4 Orthographic(float znear, float zfar, float right, float left, float top, float bottom);
            static Matrix4 BuildViewMatrix(const Vector3 &from, const Vector3 &lookingAt, const Vector3 up = Vector3(0, 1, 0));

            Matrix4 GetRotation() const;
            Matrix4 GetTransposedRotation() const;

            inline Vector4 GetRow(unsigned int row) const
            {
                return Vector4(values[row], values[row + 4], values[row + 8], values[row + 12]);
            }

            inline void SetRow(unsigned int row, const Vector4& val)
            {
                values[row] = val.GetX();
                values[row + 4] = val.GetY();
                values[row + 8] = val.GetZ();
                values[row + 12] = val.GetW();
            }

            inline Matrix4 operator*(const Matrix4 &a) const
            {
                Matrix4 out;

                for (unsigned int r = 0; r < 4; ++r)
                {
                    for (unsigned int c = 0; c < 4; ++c)
                    {
                        float sum = 0.0f;

                        for (unsigned int i = 0; i < 4; ++i)
                        {
                            sum += this->values[c + (i * 4)] * a.values[(r * 4) + i];
                        }
                        out.values[c + (r * 4)] = sum;
                    }
                }
                return out;
            }

            inline Vector3 operator*(const Vector3 &v) const
            {
                Vector3 vec;

                vec.SetX(v.GetX()*values[0] + v.GetY()*values[4] + v.GetZ()*values[8] + values[12]);
                vec.SetY(v.GetX()*values[1] + v.GetY()*values[5] + v.GetZ()*values[9] + values[13]);
                vec.SetZ(v.GetX()*values[2] + v.GetY()*values[6] + v.GetZ()*values[10] + values[14]);

                const float temp = v.GetX()*values[3] + v.GetY()*values[7] + v.GetZ()*values[11] + values[15];

                vec.SetX(vec.GetX() / temp);
                vec.SetY(vec.GetY() / temp);
                vec.SetZ(vec.GetZ() / temp);

                return vec;
            };

            inline Vector4 operator*(const Vector4 &v) const
            {
                return Vector4(
                    v.GetX()*values[0] + v.GetY()*values[4] + v.GetZ()*values[8]  + v.GetW() * values[12],
                    v.GetX()*values[1] + v.GetY()*values[5] + v.GetZ()*values[9]  + v.GetW() * values[13],
                    v.GetX()*values[2] + v.GetY()*values[6] + v.GetZ()*values[10] + v.GetW() * values[14],
                    v.GetX()*values[3] + v.GetY()*values[7] + v.GetZ()*values[11] + v.GetW() * values[15]
                );
            };

            void GetMatrix(float* m) const
            {
                m[0] = this->values[0]; m[1] = this->values[1]; m[2] = this->values[2]; m[3] = this->values[3];
                m[4] = this->values[4]; m[5] = this->values[5]; m[6] = this->values[6]; m[7] = this->values[7];
                m[8] = this->values[8]; m[9] = this->values[9]; m[10] = this->values[10]; m[11] = this->values[11];
                m[12] = this->values[12]; m[13] = this->values[13]; m[14] = this->values[14]; m[15] = this->values[15];
            }

            inline friend std::ostream& operator<<(std::ostream& o, const Matrix4& m)
            {
                o << "Mat4(";
                o << "\t" << m.values[0] << "," << m.values[1] << "," << m.values[2] << "," << m.values[3] << std::endl;
                o << "\t\t" << m.values[4] << "," << m.values[5] << "," << m.values[6] << "," << m.values[7] << std::endl;
                o << "\t\t" << m.values[8] << "," << m.values[9] << "," << m.values[10] << "," << m.values[11] << std::endl;
                o << "\t\t" << m.values[12] << "," << m.values[13] << "," << m.values[14] << "," << m.values[15] << " )" << std::endl;
                return o;
            }

            inline float  operator[](const int index) const
            {
                return values[index];
            }

            inline float&  operator[](const int index)
            {
                return values[index];
            }

            static Matrix4 Inverse(const Matrix4& rhs)
            {
                Matrix4 inv;

                inv[0] = rhs[5] * rhs[10] * rhs[15] -
                    rhs[5] * rhs[11] * rhs[14] -
                    rhs[9] * rhs[6] * rhs[15] +
                    rhs[9] * rhs[7] * rhs[14] +
                    rhs[13] * rhs[6] * rhs[11] -
                    rhs[13] * rhs[7] * rhs[10];

                inv[4] = -rhs[4] * rhs[10] * rhs[15] +
                    rhs[4] * rhs[11] * rhs[14] +
                    rhs[8] * rhs[6] * rhs[15] -
                    rhs[8] * rhs[7] * rhs[14] -
                    rhs[12] * rhs[6] * rhs[11] +
                    rhs[12] * rhs[7] * rhs[10];

                inv[8] = rhs[4] * rhs[9] * rhs[15] -
                    rhs[4] * rhs[11] * rhs[13] -
                    rhs[8] * rhs[5] * rhs[15] +
                    rhs[8] * rhs[7] * rhs[13] +
                    rhs[12] * rhs[5] * rhs[11] -
                    rhs[12] * rhs[7] * rhs[9];

                inv[12] = -rhs[4] * rhs[9] * rhs[14] +
                    rhs[4] * rhs[10] * rhs[13] +
                    rhs[8] * rhs[5] * rhs[14] -
                    rhs[8] * rhs[6] * rhs[13] -
                    rhs[12] * rhs[5] * rhs[10] +
                    rhs[12] * rhs[6] * rhs[9];

                inv[1] = -rhs[1] * rhs[10] * rhs[15] +
                    rhs[1] * rhs[11] * rhs[14] +
                    rhs[9] * rhs[2] * rhs[15] -
                    rhs[9] * rhs[3] * rhs[14] -
                    rhs[13] * rhs[2] * rhs[11] +
                    rhs[13] * rhs[3] * rhs[10];

                inv[5] = rhs[0] * rhs[10] * rhs[15] -
                    rhs[0] * rhs[11] * rhs[14] -
                    rhs[8] * rhs[2] * rhs[15] +
                    rhs[8] * rhs[3] * rhs[14] +
                    rhs[12] * rhs[2] * rhs[11] -
                    rhs[12] * rhs[3] * rhs[10];

                inv[9] = -rhs[0] * rhs[9] * rhs[15] +
                    rhs[0] * rhs[11] * rhs[13] +
                    rhs[8] * rhs[1] * rhs[15] -
                    rhs[8] * rhs[3] * rhs[13] -
                    rhs[12] * rhs[1] * rhs[11] +
                    rhs[12] * rhs[3] * rhs[9];

                inv[13] = rhs[0] * rhs[9] * rhs[14] -
                    rhs[0] * rhs[10] * rhs[13] -
                    rhs[8] * rhs[1] * rhs[14] +
                    rhs[8] * rhs[2] * rhs[13] +
                    rhs[12] * rhs[1] * rhs[10] -
                    rhs[12] * rhs[2] * rhs[9];

                inv[2] = rhs[1] * rhs[6] * rhs[15] -
                    rhs[1] * rhs[7] * rhs[14] -
                    rhs[5] * rhs[2] * rhs[15] +
                    rhs[5] * rhs[3] * rhs[14] +
                    rhs[13] * rhs[2] * rhs[7] -
                    rhs[13] * rhs[3] * rhs[6];

                inv[6] = -rhs[0] * rhs[6] * rhs[15] +
                    rhs[0] * rhs[7] * rhs[14] +
                    rhs[4] * rhs[2] * rhs[15] -
                    rhs[4] * rhs[3] * rhs[14] -
                    rhs[12] * rhs[2] * rhs[7] +
                    rhs[12] * rhs[3] * rhs[6];

                inv[10] = rhs[0] * rhs[5] * rhs[15] -
                    rhs[0] * rhs[7] * rhs[13] -
                    rhs[4] * rhs[1] * rhs[15] +
                    rhs[4] * rhs[3] * rhs[13] +
                    rhs[12] * rhs[1] * rhs[7] -
                    rhs[12] * rhs[3] * rhs[5];

                inv[14] = -rhs[0] * rhs[5] * rhs[14] +
                    rhs[0] * rhs[6] * rhs[13] +
                    rhs[4] * rhs[1] * rhs[14] -
                    rhs[4] * rhs[2] * rhs[13] -
                    rhs[12] * rhs[1] * rhs[6] +
                    rhs[12] * rhs[2] * rhs[5];

                inv[3] = -rhs[1] * rhs[6] * rhs[11] +
                    rhs[1] * rhs[7] * rhs[10] +
                    rhs[5] * rhs[2] * rhs[11] -
                    rhs[5] * rhs[3] * rhs[10] -
                    rhs[9] * rhs[2] * rhs[7] +
                    rhs[9] * rhs[3] * rhs[6];

                inv[7] = rhs[0] * rhs[6] * rhs[11] -
                    rhs[0] * rhs[7] * rhs[10] -
                    rhs[4] * rhs[2] * rhs[11] +
                    rhs[4] * rhs[3] * rhs[10] +
                    rhs[8] * rhs[2] * rhs[7] -
                    rhs[8] * rhs[3] * rhs[6];

                inv[11] = -rhs[0] * rhs[5] * rhs[11] +
                    rhs[0] * rhs[7] * rhs[9] +
                    rhs[4] * rhs[1] * rhs[11] -
                    rhs[4] * rhs[3] * rhs[9] -
                    rhs[8] * rhs[1] * rhs[7] +
                    rhs[8] * rhs[3] * rhs[5];

                inv[15] = rhs[0] * rhs[5] * rhs[10] -
                    rhs[0] * rhs[6] * rhs[9] -
                    rhs[4] * rhs[1] * rhs[10] +
                    rhs[4] * rhs[2] * rhs[9] +
                    rhs[8] * rhs[1] * rhs[6] -
                    rhs[8] * rhs[2] * rhs[5];

                float det = rhs[0] * inv[0] + rhs[1] * inv[4] + rhs[2] * inv[8] + rhs[3] * inv[12];

                if (det == 0)
                {
                    inv.ToIdentity();
                    return inv;
                }

                det = 1.f / det;

                for (int i = 0; i < 16; i++)
                {
                    inv[i] = inv[i] * det;
                }

                return inv;
            }
        };

#endif
	}
}