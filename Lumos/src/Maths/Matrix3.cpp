#include "JM.h"
#include "Matrix3.h"
#include "Matrix4.h"

#include "MathsUtilities.h"

namespace jm
{
	namespace maths {

#ifdef JM_SSEMAT3

		const float Matrix3::EMPTY_DATA[9] =
				{
						0, 0, 0,
						0, 0, 0,
						0, 0, 0
				};

		const float Matrix3::IDENTITY_DATA[9] =
				{
						1, 0, 0,
						0, 1, 0,
						0, 0, 1
				};

		const Matrix3 Matrix3::ZeroMatrix = Matrix3(Matrix3::EMPTY_DATA);
		const Matrix3 Matrix3::EMPTY = Matrix3(Matrix3::EMPTY_DATA);
		const Matrix3 Matrix3::IDENTITY = Matrix3(Matrix3::IDENTITY_DATA);

		Matrix3::Matrix3(
				const Vector3 &v1,
				const Vector3 &v2,
				const Vector3 &v3) {
#ifdef JM_SSEVEC3
			const unsigned int size = 3 * sizeof(float);
			memcpy(&values[0], &v1.x, size);
			memcpy(&values[3], &v2.x, size);
			memcpy(&values[6], &v3.x, size);
#else
            _11 = v1.GetX(); _12 = v1.GetY(); _13 = v1.GetZ();
            _21 = v2.GetX(); _22 = v2.GetY(); _23 = v2.GetZ();
            _31 = v3.GetX(); _32 = v3.GetY(); _33 = v3.GetZ();
#endif
		}

		Matrix3::Matrix3(
				float a1, float a2, float a3,
				float b1, float b2, float b3,
				float c1, float c2, float c3) :
				_11(a1), _12(a2), _13(a3),
				_21(b1), _22(b2), _23(b3),
				_31(c1), _32(c2), _33(c3) {}


		Matrix3::Matrix3(const Matrix4 &m4) {
#ifdef JM_SSEMAT4
			_mm_storeu_ps(&values[0], m4.mmvalues[0]);
			_mm_storeu_ps(&values[3], m4.mmvalues[1]);
			values[6] = m4.values[8];
			values[7] = m4.values[9];
			values[8] = m4.values[10];
#else
            const unsigned int size = 3 * sizeof(float);
            memcpy(&values[0], &m4.values[0], size);
            memcpy(&values[3], &m4.values[4], size);
            memcpy(&values[6], &m4.values[8], size);
#endif
		}


		Matrix3 Matrix3::Rotation(float degrees, const Vector3 &axis) {
			Matrix3 m;
			Vector3 axisNorm = axis;
			axisNorm.Normalise();

			float c = cos(jm::maths::DegToRad(degrees));
			float s = sin(jm::maths::DegToRad(degrees));

			__m128 normXYZW = _mm_set_ps(0, axisNorm.GetZ(), axisNorm.GetY(), axisNorm.GetX());
			__m128 normXYZWWithC = _mm_mul_ps(normXYZW, _mm_set1_ps(1.0f - c));
			__m128 col0 = _mm_mul_ps(normXYZW, _mm_set1_ps(GetValue(normXYZWWithC, 0)));
			__m128 col1 = _mm_mul_ps(normXYZW, _mm_set1_ps(GetValue(normXYZWWithC, 1)));
			__m128 col2 = _mm_mul_ps(normXYZW, _mm_set1_ps(GetValue(normXYZWWithC, 2)));

			m.values[0] = GetValue(col0, 0) + c;
			m.values[1] = GetValue(col0, 1) + (axisNorm.GetZ() * s);
			m.values[2] = GetValue(col0, 2) - (axisNorm.GetY() * s);
			m.values[3] = GetValue(col1, 0) - (axisNorm.GetZ() * s);
			m.values[4] = GetValue(col1, 1) + c;
			m.values[5] = GetValue(col1, 2) + (axisNorm.GetX() * s);
			m.values[6] = GetValue(col2, 0) + (axisNorm.GetY() * s);
			m.values[7] = GetValue(col2, 1) - (axisNorm.GetX() * s);
			m.values[8] = GetValue(col2, 2) + c;
			return m;
		}

		Matrix3 Matrix3::Rotation(float degreesX, float degreesY, float degreesZ) {
			// Building this matrix directly is faster than multiplying three matrices for X, Y and Z
			float phi = jm::maths::DegToRad(degreesX), theta = jm::maths::DegToRad(degreesY), psi = jm::maths::DegToRad(
					degreesZ);
			float sinTh = sin(theta), cosTh = cos(theta),
					sinPh = sin(phi), cosPh = cos(phi),
					sinPs = sin(psi), cosPs = cos(psi);

			Matrix3 result;
			result.values[0] = cosTh * cosPs;
			result.values[1] = cosTh * sinPs;
			result.values[2] = -sinTh;
			result.values[3] = -cosPh * sinPs + sinPh * sinTh * cosPs;
			result.values[4] = cosPh * cosPs + sinPh * sinTh * sinPs;
			result.values[5] = sinPh * cosTh;
			result.values[6] = sinPh * sinPs + cosPh * sinTh * cosPs;
			result.values[7] = -sinPh * cosPs + cosPh * sinTh * sinPs;
			result.values[8] = cosPh * cosTh;
			return result;
		}

		Matrix3 Matrix3::RotationX(float degrees) {
			Matrix3 m;
			float rad = jm::maths::DegToRad(degrees);
			float c = cos(rad);
			float s = sin(rad);

			m.values[4] = c;
			m.values[5] = s;

			m.values[7] = -s;
			m.values[8] = c;

			return m;
		}

		Matrix3 Matrix3::RotationY(float degrees) {
			Matrix4 m;
			float rad = jm::maths::DegToRad(degrees);
			float c = cos(rad);
			float s = sin(rad);

			m.values[0] = c;
			m.values[2] = s;

			m.values[6] = -s;
			m.values[8] = c;

			return m;
		}

		Matrix3 Matrix3::RotationZ(float degrees) {
			Matrix4 m;
			float rad = jm::maths::DegToRad(degrees);
			float c = cos(rad);
			float s = sin(rad);

			m.values[0] = c;
			m.values[1] = -s;

			m.values[3] = s;
			m.values[4] = c;

			return m;
		}

		Matrix3 Matrix3::Scale(const Vector3 &scale) {
			Matrix3 m;
			m.SetDiagonal(scale);
			return m;
		}


		float Matrix3::Determinant() const {
			return
					values[0] * (values[4] * values[8] - values[5] * values[7]) -
					values[3] * (values[1] * values[8] - values[7] * values[2]) +
					values[6] * (values[1] * values[5] - values[4] * values[2]);
		}

		Matrix3 Matrix3::Inverse() const {
			Matrix3 result;
			float det = Determinant();
			if (det != 0.0f) {
				float detReciprocal = 1.0f / det;
				result.values[0] = (values[4] * values[8] - values[5] * values[7]) * detReciprocal;
				result.values[1] = (values[7] * values[2] - values[1] * values[8]) * detReciprocal;
				result.values[2] = (values[1] * values[5] - values[2] * values[4]) * detReciprocal;
				result.values[3] = (values[6] * values[5] - values[3] * values[8]) * detReciprocal;
				result.values[4] = (values[0] * values[8] - values[6] * values[2]) * detReciprocal;
				result.values[5] = (values[2] * values[3] - values[0] * values[5]) * detReciprocal;
				result.values[6] = (values[3] * values[7] - values[6] * values[4]) * detReciprocal;
				result.values[7] = (values[1] * values[6] - values[0] * values[7]) * detReciprocal;
				result.values[8] = (values[0] * values[4] - values[1] * values[3]) * detReciprocal;
			}
			return result;
		}

		bool Matrix3::TryInvert() {
			float det = Determinant();
			if (det != 0.0f) {
				float detReciprocal = 1.0f / det;
				float result[9];
				result[0] = (values[4] * values[8] - values[5] * values[7]) * detReciprocal;
				result[1] = (values[7] * values[2] - values[1] * values[8]) * detReciprocal;
				result[2] = (values[1] * values[5] - values[2] * values[4]) * detReciprocal;
				result[3] = (values[6] * values[5] - values[3] * values[8]) * detReciprocal;
				result[4] = (values[0] * values[8] - values[6] * values[2]) * detReciprocal;
				result[5] = (values[2] * values[3] - values[0] * values[5]) * detReciprocal;
				result[6] = (values[3] * values[7] - values[6] * values[4]) * detReciprocal;
				result[7] = (values[1] * values[6] - values[0] * values[7]) * detReciprocal;
				result[8] = (values[0] * values[4] - values[1] * values[3]) * detReciprocal;
				memcpy(values, result, 9 * sizeof(float));
				return true;
			}
			return false;
		}

		bool Matrix3::TryTransposedInvert() {
			float det = Determinant();
			if (det != 0.0f) {
				float invdet = 1.0f / det;
				float result[9];
				result[0] = (values[4] * values[8] - values[5] * values[7]) * invdet;
				result[1] = (values[6] * values[5] - values[3] * values[8]) * invdet;
				result[2] = (values[3] * values[7] - values[6] * values[4]) * invdet;
				result[3] = (values[7] * values[2] - values[1] * values[8]) * invdet;
				result[4] = (values[0] * values[8] - values[6] * values[2]) * invdet;
				result[5] = (values[1] * values[6] - values[0] * values[7]) * invdet;
				result[6] = (values[1] * values[5] - values[2] * values[4]) * invdet;
				result[7] = (values[2] * values[3] - values[0] * values[5]) * invdet;
				result[8] = (values[0] * values[4] - values[1] * values[3]) * invdet;
				memcpy(values, result, 9 * sizeof(float));
				return true;
			}
			return false;
		}

// Standard Matrix Functionality
		Matrix3 Matrix3::Inverse(const Matrix3 &rhs) {
			Matrix3 out;
			const float det = rhs.Determinant();
			if (det != 0.f) {
				const float invdet = 1.0f / det;
				out(0, 0) = (rhs(1, 1) * rhs(2, 2) - rhs(2, 1) * rhs(1, 2)) * invdet;
				out(0, 1) = -(rhs(0, 1) * rhs(2, 2) - rhs(0, 2) * rhs(2, 1)) * invdet;
				out(0, 2) = (rhs(0, 1) * rhs(1, 2) - rhs(0, 2) * rhs(1, 1)) * invdet;
				out(1, 0) = -(rhs(1, 0) * rhs(2, 2) - rhs(1, 2) * rhs(2, 0)) * invdet;
				out(1, 1) = (rhs(0, 0) * rhs(2, 2) - rhs(0, 2) * rhs(2, 0)) * invdet;
				out(1, 2) = -(rhs(0, 0) * rhs(1, 2) - rhs(1, 0) * rhs(0, 2)) * invdet;
				out(2, 0) = (rhs(1, 0) * rhs(2, 1) - rhs(2, 0) * rhs(1, 1)) * invdet;
				out(2, 1) = -(rhs(0, 0) * rhs(2, 1) - rhs(2, 0) * rhs(0, 1)) * invdet;
				out(2, 2) = (rhs(0, 0) * rhs(1, 1) - rhs(1, 0) * rhs(0, 1)) * invdet;
			}
			return out;
		}

		Matrix3 Matrix3::Transpose(const Matrix3 &rhs) {
			Matrix3 m;

			m._11 = rhs._11;
			m._21 = rhs._12;
			m._31 = rhs._13;

			m._12 = rhs._21;
			m._22 = rhs._22;
			m._32 = rhs._23;

			m._13 = rhs._31;
			m._23 = rhs._32;
			m._33 = rhs._33;

			return m;
		}

		Matrix3 Matrix3::Adjugate(const Matrix3 &m) {
			Matrix3 adj;

			adj._11 = m._22 * m._33 - m._23 * m._32;
			adj._12 = m._13 * m._32 - m._12 * m._33;
			adj._13 = m._12 * m._23 - m._13 * m._22;

			adj._21 = m._23 * m._31 - m._21 * m._33;
			adj._22 = m._11 * m._33 - m._13 * m._31;
			adj._23 = m._13 * m._21 - m._11 * m._23;

			adj._31 = m._21 * m._32 - m._22 * m._31;
			adj._32 = m._12 * m._31 - m._11 * m._32;
			adj._33 = m._11 * m._22 - m._12 * m._21;

			return adj;
		}


		std::ostream &operator<<(std::ostream &o, const Matrix3 &m) {
			return o << "Mat3(" << "/n" <<
					 "\t" << m.values[0] << ", " << m.values[3] << ", " << m.values[6] << ", " << "/n" <<
					 "\t" << m.values[1] << ", " << m.values[4] << ", " << m.values[7] << ", " << "/n" <<
					 "\t" << m.values[2] << ", " << m.values[5] << ", " << m.values[8] << "/n" <<
					 ")";
		}

#else

        const Matrix3 Matrix3::Identity = Matrix3();
        const Matrix3 Matrix3::ZeroMatrix = Matrix3(0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f);

        //ctor
        Matrix3::Matrix3() :
            _11(1.0f), _12(0.0f), _13(0.0f),
            _21(0.0f), _22(1.0f), _23(0.0f),
            _31(0.0f), _32(0.0f), _33(1.0f)
        {
        }

        Matrix3::Matrix3(const float e[9]) :
            _11(e[0]), _12(e[1]), _13(e[2]),
            _21(e[3]), _22(e[4]), _23(e[5]),
            _31(e[6]), _32(e[7]), _33(e[8])
        {
        }

        Matrix3::Matrix3(const Vector3& c1, const Vector3& c2, const Vector3& c3)
        {
            //const unsigned int size = 3 * sizeof(float);
            //memcpy(&mat_array[0], &c1.GetX(), size);
            //memcpy(&mat_array[3], &c2.GetY(), size);
            //memcpy(&mat_array[6], &c3.GetZ(), size);

            _11 = c1.GetX(); _12 = c1.GetY(); _13 = c1.GetZ();
            _21 = c2.GetX(); _22 = c2.GetY(); _23 = c2.GetZ();
            _31 = c3.GetX(); _32 = c3.GetY(); _33 = c3.GetZ();
        }

        Matrix3::Matrix3(const float a1, const float a2, const float a3,
                         const float b1, const float b2, const float b3,
                         const float c1, const float c2, const float c3) :
            _11(a1), _12(a2), _13(a3),
            _21(b1), _22(b2), _23(b3),
            _31(c1), _32(c2), _33(c3)
        {
        }

        Matrix3::Matrix3(const Matrix4& mat44)
        {
            const unsigned int size = 3 * sizeof(float);
            memcpy(&values[0], &mat44.values[0], size);
            memcpy(&values[3], &mat44.values[4], size);
            memcpy(&values[6], &mat44.values[8], size);
        }

        //Default States
        void Matrix3::ToZero()
        {
            memset(values, 0, 9 * sizeof(float));
        }

        void Matrix3::ToIdentity()
        {
            _11 = 1.0f; _12 = 0.0f; _13 = 0.0f;
            _21 = 0.0f; _22 = 1.0f; _23 = 0.0f;
            _31 = 0.0f; _32 = 0.0f; _33 = 1.0f;
        }

        //Transformation Matrix
        Matrix3 Matrix3::Rotation(const float degrees, const Vector3 &inaxis)
        {
            Matrix3 m;

            Vector3 axis = inaxis;
            axis.Normalise();

            const float c = cosf(static_cast<float>(maths::DegToRad(degrees)));
            const float s = sinf(static_cast<float>(maths::DegToRad(degrees)));

            m(0, 0) = (axis.GetX() * axis.GetX()) * (1.0f - c) + c;
            m(1, 0) = (axis.GetY() * axis.GetX()) * (1.0f - c) + (axis.GetZ() * s);
            m(2, 0) = (axis.GetZ() * axis.GetX()) * (1.0f - c) - (axis.GetY() * s);

            m(0, 1) = (axis.GetX() * axis.GetY()) * (1.0f - c) - (axis.GetZ() * s);
            m(1, 1) = (axis.GetY() * axis.GetY()) * (1.0f - c) + c;
            m(2, 1) = (axis.GetZ() * axis.GetY()) * (1.0f - c) + (axis.GetX() * s);

            m(0, 2) = (axis.GetX() * axis.GetZ()) * (1.0f - c) + (axis.GetY() * s);
            m(1, 2) = (axis.GetY() * axis.GetZ()) * (1.0f - c) - (axis.GetX() * s);
            m(2, 2) = (axis.GetZ() * axis.GetZ()) * (1.0f - c) + c;

            return m;
        }

        Matrix3 Matrix3::Scale(const Vector3 &scale)
        {
            Matrix3 m;
            m.SetScalingVector(scale);
            return m;
        }

        // Standard Matrix Functionality
        Matrix3 Matrix3::Inverse(const Matrix3& rhs)
        {
            Matrix3 out;
            const float det = rhs.Determinant();
            if (det != 0.f)
            {
                const float invdet = 1.0f / det;
                out(0, 0) = (rhs(1, 1)*rhs(2, 2) - rhs(2, 1)*rhs(1, 2))*invdet;
                out(0, 1) = -(rhs(0, 1)*rhs(2, 2) - rhs(0, 2)*rhs(2, 1))*invdet;
                out(0, 2) = (rhs(0, 1)*rhs(1, 2) - rhs(0, 2)*rhs(1, 1))*invdet;
                out(1, 0) = -(rhs(1, 0)*rhs(2, 2) - rhs(1, 2)*rhs(2, 0))*invdet;
                out(1, 1) = (rhs(0, 0)*rhs(2, 2) - rhs(0, 2)*rhs(2, 0))*invdet;
                out(1, 2) = -(rhs(0, 0)*rhs(1, 2) - rhs(1, 0)*rhs(0, 2))*invdet;
                out(2, 0) = (rhs(1, 0)*rhs(2, 1) - rhs(2, 0)*rhs(1, 1))*invdet;
                out(2, 1) = -(rhs(0, 0)*rhs(2, 1) - rhs(2, 0)*rhs(0, 1))*invdet;
                out(2, 2) = (rhs(0, 0)*rhs(1, 1) - rhs(1, 0)*rhs(0, 1))*invdet;
            }
            return out;
        }

        Matrix3 Matrix3::Transpose(const Matrix3& rhs)
        {
            Matrix3 m;

            m._11 = rhs._11;
            m._21 = rhs._12;
            m._31 = rhs._13;

            m._12 = rhs._21;
            m._22 = rhs._22;
            m._32 = rhs._23;

            m._13 = rhs._31;
            m._23 = rhs._32;
            m._33 = rhs._33;

            return m;
        }

        Matrix3 Matrix3::Adjugate(const Matrix3& m)
        {
            Matrix3 adj;

            adj._11 = m._22 * m._33 - m._23 * m._32;
            adj._12 = m._13 * m._32 - m._12 * m._33;
            adj._13 = m._12 * m._23 - m._13 * m._22;

            adj._21 = m._23 * m._31 - m._21 * m._33;
            adj._22 = m._11 * m._33 - m._13 * m._31;
            adj._23 = m._13 * m._21 - m._11 * m._23;

            adj._31 = m._21 * m._32 - m._22 * m._31;
            adj._32 = m._12 * m._31 - m._11 * m._32;
            adj._33 = m._11 * m._22 - m._12 * m._21;

            return adj;
        }

        Matrix3 Matrix3::OuterProduct(const Vector3& a, const Vector3& b)
        {
            Matrix3 m;

            m._11 = a.GetX() * b.GetX();
            m._12 = a.GetX() * b.GetY();
            m._13 = a.GetX() * b.GetZ();

            m._21 = a.GetY() * b.GetX();
            m._22 = a.GetY() * b.GetY();
            m._23 = a.GetY() * b.GetZ();

            m._31 = a.GetZ() * b.GetX();
            m._32 = a.GetZ() * b.GetY();
            m._33 = a.GetZ() * b.GetZ();

            return m;
        }

        // Additional Functionality
        float Matrix3::Trace() const
        {
            return _11 + _22 + _33;
        }

        float Matrix3::Determinant() const
        {
            return +_11*(_22*_33 - _32*_23) - _12*(_21*_33 - _23*_31) + _13*(_21*_32 - _22*_31);
        }

        Matrix3& operator+=(Matrix3& a, const Matrix3& b)
        {
            for (unsigned int i = 0; i < 9; ++i)
                a.values[i] += b.values[i];
            return a;
        }

        Matrix3& operator-=(Matrix3& a, const Matrix3& b)
        {
            for (unsigned int i = 0; i < 9; ++i)
                a.values[i] -= b.values[i];
            return a;
        }

        Matrix3 operator+(const Matrix3& a, const Matrix3& b)
        {
            Matrix3 m;
            for (unsigned int i = 0; i < 9; ++i)
                m.values[i] = a.values[i] + b.values[i];
            return m;
        }

        Matrix3 operator-(const Matrix3& a, const Matrix3& b)
        {
            Matrix3 m;
            for (unsigned int i = 0; i < 9; ++i)
                m.values[i] = a.values[i] - b.values[i];
            return m;
        }

        Matrix3 operator*(const Matrix3& a, const Matrix3& b)
        {
            Matrix3 out;

            out._11 = a._11 * b._11 + a._12 * b._21 + a._13 * b._31;
            out._12 = a._11 * b._12 + a._12 * b._22 + a._13 * b._32;
            out._13 = a._11 * b._13 + a._12 * b._23 + a._13 * b._33;

            out._21 = a._21 * b._11 + a._22 * b._21 + a._23 * b._31;
            out._22 = a._21 * b._12 + a._22 * b._22 + a._23 * b._32;
            out._23 = a._21 * b._13 + a._22 * b._23 + a._23 * b._33;

            out._31 = a._31 * b._11 + a._32 * b._21 + a._33 * b._31;
            out._32 = a._31 * b._12 + a._32 * b._22 + a._33 * b._32;
            out._33 = a._31 * b._13 + a._32 * b._23 + a._33 * b._33;

            return out;
        }

        Matrix3& operator+=(Matrix3& a, const float b)
        {
            for (unsigned int i = 0; i < 9; ++i)
                a.values[i] += b;
            return a;
        }

        Matrix3& operator-=(Matrix3& a, const float b)
        {
            for (unsigned int i = 0; i < 9; ++i)
                a.values[i] -= b;
            return a;
        }
        Matrix3& operator*=(Matrix3& a, const float b)
        {
            for (unsigned int i = 0; i < 9; ++i)
                a.values[i] *= b;
            return a;
        }
        Matrix3& operator/=(Matrix3& a, const float b)
        {
            for (unsigned int i = 0; i < 9; ++i)
                a.values[i] /= b;
            return a;
        }

        Matrix3 operator+(Matrix3& a, const float b)
        {
            Matrix3 m;
            for (unsigned int i = 0; i < 9; ++i)
                m.values[i] = a.values[i] + b;
            return m;
        }

        Matrix3 operator-(const Matrix3& a, const float b)
        {
            Matrix3 m;
            for (unsigned int i = 0; i < 9; ++i)
                m.values[i] = a.values[i] - b;
            return m;
        }
        Matrix3 operator*(const Matrix3& a, const float b)
        {
            Matrix3 m;
            for (unsigned int i = 0; i < 9; ++i)
                m.values[i] = a.values[i] * b;
            return m;
        }
        Matrix3 operator/(const Matrix3& a, const float b)
        {
            Matrix3 m;
            for (unsigned int i = 0; i < 9; ++i)
                m.values[i] = a.values[i] / b;
            return m;
        }

        Vector3 operator*(const Matrix3& a, const Vector3& b)
        {
            Vector3 out;

            out.SetX( a._11 * b.GetX()
                    + a._21 * b.GetY()
                    + a._31 * b.GetZ());

            out.SetY( a._12 * b.GetX()
                    + a._22 * b.GetY()
                    + a._32 * b.GetZ());

            out.SetZ( a._13 * b.GetX()
                    + a._23 * b.GetY()
                    + a._33 * b.GetZ());

            return out;
        }

#endif

	}
}
