#include "Precompiled.h"
#include "Matrix3.h"
#include "Matrix4.h"
#include "MathsUtilities.h"
#include "SSEUtilities.h"

namespace Lumos
{
    namespace Maths
    {
        const float Matrix3::EMPTY_DATA[9] = {
            0, 0, 0,
            0, 0, 0,
            0, 0, 0
        };

        const float Matrix3::IDENTITY_DATA[9] = {
            1, 0, 0,
            0, 1, 0,
            0, 0, 1
        };

        const Matrix3 Matrix3::ZeroMatrix = Matrix3(Matrix3::EMPTY_DATA);
        const Matrix3 Matrix3::EMPTY      = Matrix3(Matrix3::EMPTY_DATA);
        const Matrix3 Matrix3::IDENTITY   = Matrix3(Matrix3::IDENTITY_DATA);

        Matrix3::Matrix3(
            const Vector3& v1,
            const Vector3& v2,
            const Vector3& v3)
        {
#ifdef LUMOS_SSE
            const unsigned int size = 3 * sizeof(float);
            memcpy(&values[0], &v1.x, size);
            memcpy(&values[3], &v2.x, size);
            memcpy(&values[6], &v3.x, size);
#else
            _11 = v1.x;
            _12 = v1.y;
            _13 = v1.z;
            _21 = v2.x;
            _22 = v2.y;
            _23 = v2.z;
            _31 = v3.x;
            _32 = v3.y;
            _33 = v3.z;
#endif
        }

        Matrix3::Matrix3(const float (&elements)[9])
        {
            MemoryCopy(values, elements, 9 * sizeof(float));
        }

        Matrix3::Matrix3(
            float a1, float a2, float a3,
            float b1, float b2, float b3,
            float c1, float c2, float c3)
            : _11(a1)
            , _12(a2)
            , _13(a3)
            , _21(b1)
            , _22(b2)
            , _23(b3)
            , _31(c1)
            , _32(c2)
            , _33(c3)
        {
        }

        Matrix3::Matrix3(const Matrix4& m4)
        {
#ifdef LUMOS_SSE
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

        Matrix3::Matrix3(const Quaternion& q)
        {
            *this = q.ToMatrix3();
        }

        Matrix3& Matrix3::ToZero()
        {
            MemorySet(values, 0, 9 * sizeof(float));
            return *this;
        }

        Matrix3& Matrix3::ToIdentity()
        {
            MemoryCopy(values, Matrix3::IDENTITY_DATA, 9 * sizeof(float));
            return *this;
        }

        Matrix3 Matrix3::Rotation(float degrees, const Vector3& axis)
        {
            Matrix3 m;
            Vector3 axisNorm = axis;
            axisNorm.Normalise();

            float c = cos(Maths::ToRadians(degrees));
            float s = sin(Maths::ToRadians(degrees));

#ifdef LUMOS_SSE
            __m128 normXYZW      = _mm_set_ps(0, axisNorm.z, axisNorm.y, axisNorm.x);
            __m128 normXYZWWithC = _mm_mul_ps(normXYZW, _mm_set1_ps(1.0f - c));
            __m128 col0          = _mm_mul_ps(normXYZW, _mm_set1_ps(GetValue(normXYZWWithC, 0)));
            __m128 col1          = _mm_mul_ps(normXYZW, _mm_set1_ps(GetValue(normXYZWWithC, 1)));
            __m128 col2          = _mm_mul_ps(normXYZW, _mm_set1_ps(GetValue(normXYZWWithC, 2)));

            m.values[0] = GetValue(col0, 0) + c;
            m.values[1] = GetValue(col0, 1) + (axisNorm.z * s);
            m.values[2] = GetValue(col0, 2) - (axisNorm.y * s);
            m.values[3] = GetValue(col1, 0) - (axisNorm.z * s);
            m.values[4] = GetValue(col1, 1) + c;
            m.values[5] = GetValue(col1, 2) + (axisNorm.x * s);
            m.values[6] = GetValue(col2, 0) + (axisNorm.y * s);
            m.values[7] = GetValue(col2, 1) - (axisNorm.x * s);
            m.values[8] = GetValue(col2, 2) + c;
#else
            m(0, 0) = (axis.x * axis.x) * (1.0f - c) + c;
            m(1, 0) = (axis.y * axis.x) * (1.0f - c) + (axis.z * s);
            m(2, 0) = (axis.z * axis.x) * (1.0f - c) - (axis.y * s);

            m(0, 1) = (axis.x * axis.y) * (1.0f - c) - (axis.z * s);
            m(1, 1) = (axis.y * axis.y) * (1.0f - c) + c;
            m(2, 1) = (axis.z * axis.y) * (1.0f - c) + (axis.x * s);

            m(0, 2) = (axis.x * axis.z) * (1.0f - c) + (axis.y * s);
            m(1, 2) = (axis.y * axis.z) * (1.0f - c) - (axis.x * s);
            m(2, 2) = (axis.z * axis.z) * (1.0f - c) + c;
#endif
            return m;
        }

        void Matrix3::Transpose()
        {
#ifdef LUMOS_SSE
            __m128 empty   = _mm_setzero_ps();
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

        Matrix3 Matrix3::Rotation(float degreesX, float degreesY, float degreesZ)
        {
            // Building this matrix directly is faster than multiplying three matrices for X, Y and Z
            float phi = Lumos::Maths::ToRadians(degreesX), theta = Maths::ToRadians(degreesY), psi = Maths::ToRadians(degreesZ);
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

        Matrix3 Matrix3::RotationX(float degrees)
        {
            Matrix3 m;
            float rad = Lumos::Maths::ToRadians(degrees);
            float c   = cos(rad);
            float s   = sin(rad);

            m.values[4] = c;
            m.values[5] = s;

            m.values[7] = -s;
            m.values[8] = c;

            return m;
        }

        Matrix3 Matrix3::RotationY(float degrees)
        {
            Matrix3 m;
            float rad = Maths::ToRadians(degrees);
            float c   = cos(rad);
            float s   = sin(rad);

            m.values[0] = c;
            m.values[2] = s;

            m.values[6] = -s;
            m.values[8] = c;

            return m;
        }

        Matrix3 Matrix3::RotationZ(float degrees)
        {
            Matrix3 m;
            float rad = Maths::ToRadians(degrees);
            float c   = cos(rad);
            float s   = sin(rad);

            m.values[0] = c;
            m.values[1] = -s;

            m.values[3] = s;
            m.values[4] = c;

            return m;
        }

        Matrix3 Matrix3::Scale(const Vector3& scale)
        {
            Matrix3 m;
            m.SetDiagonal(scale);
            return m;
        }

        Vector3 Matrix3::operator*(const Vector3& v) const
        {
            return Vector3(
                v.x * values[0] + v.y * values[3] + v.z * values[6],
                v.x * values[1] + v.y * values[4] + v.z * values[7],
                v.x * values[2] + v.y * values[5] + v.z * values[8]);
        }

        Matrix3 Matrix3::operator*(const Matrix3& m) const
        {
            Matrix3 result;
            for(unsigned i = 0; i < 9; i += 3)
                for(unsigned j = 0; j < 3; ++j)
                    result.values[i + j] = m.values[i] * values[j] + m.values[i + 1] * values[j + 3] + m.values[i + 2] * values[j + 6];

            return result;
        }

        float Matrix3::Determinant() const
        {
            return values[0] * (values[4] * values[8] - values[5] * values[7]) - values[3] * (values[1] * values[8] - values[7] * values[2]) + values[6] * (values[1] * values[5] - values[4] * values[2]);
        }

        Matrix3 Matrix3::Inverse() const
        {
            Matrix3 result;
            float det = Determinant();
            if(det != 0.0f)
            {
                float detReciprocal = 1.0f / det;
                result.values[0]    = (values[4] * values[8] - values[5] * values[7]) * detReciprocal;
                result.values[1]    = (values[7] * values[2] - values[1] * values[8]) * detReciprocal;
                result.values[2]    = (values[1] * values[5] - values[2] * values[4]) * detReciprocal;
                result.values[3]    = (values[6] * values[5] - values[3] * values[8]) * detReciprocal;
                result.values[4]    = (values[0] * values[8] - values[6] * values[2]) * detReciprocal;
                result.values[5]    = (values[2] * values[3] - values[0] * values[5]) * detReciprocal;
                result.values[6]    = (values[3] * values[7] - values[6] * values[4]) * detReciprocal;
                result.values[7]    = (values[1] * values[6] - values[0] * values[7]) * detReciprocal;
                result.values[8]    = (values[0] * values[4] - values[1] * values[3]) * detReciprocal;
            }
            return result;
        }

        bool Matrix3::TryInvert()
        {
            float det = Determinant();
            if(det != 0.0f)
            {
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

        bool Matrix3::TryTransposedInvert()
        {
            float det = Determinant();
            if(det != 0.0f)
            {
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

        std::ostream& operator<<(std::ostream& o, const Matrix3& m)
        {
            return o << "Mat3(" << "/n" << "\t" << m.values[0] << ", " << m.values[3] << ", " << m.values[6] << ", " << "/n" << "\t" << m.values[1] << ", " << m.values[4] << ", " << m.values[7] << ", " << "/n" << "\t" << m.values[2] << ", " << m.values[5] << ", " << m.values[8] << "/n" << ")";
        }

        // Standard Matrix Functionality
        Matrix3 Matrix3::Inverse(const Matrix3& rhs)
        {
            Matrix3 out;
            const float det = rhs.Determinant();
            if(det != 0.f)
            {
                const float invdet = 1.0f / det;
                out(0, 0)          = (rhs(1, 1) * rhs(2, 2) - rhs(2, 1) * rhs(1, 2)) * invdet;
                out(0, 1)          = -(rhs(0, 1) * rhs(2, 2) - rhs(0, 2) * rhs(2, 1)) * invdet;
                out(0, 2)          = (rhs(0, 1) * rhs(1, 2) - rhs(0, 2) * rhs(1, 1)) * invdet;
                out(1, 0)          = -(rhs(1, 0) * rhs(2, 2) - rhs(1, 2) * rhs(2, 0)) * invdet;
                out(1, 1)          = (rhs(0, 0) * rhs(2, 2) - rhs(0, 2) * rhs(2, 0)) * invdet;
                out(1, 2)          = -(rhs(0, 0) * rhs(1, 2) - rhs(1, 0) * rhs(0, 2)) * invdet;
                out(2, 0)          = (rhs(1, 0) * rhs(2, 1) - rhs(2, 0) * rhs(1, 1)) * invdet;
                out(2, 1)          = -(rhs(0, 0) * rhs(2, 1) - rhs(2, 0) * rhs(0, 1)) * invdet;
                out(2, 2)          = (rhs(0, 0) * rhs(1, 1) - rhs(1, 0) * rhs(0, 1)) * invdet;
            }
            return out;
        }

        Matrix3 Matrix3::OuterProduct(const Vector3& a, const Vector3& b)
        {
            Matrix3 m;

            m._11 = a.x * b.x;
            m._12 = a.x * b.y;
            m._13 = a.x * b.z;

            m._21 = a.y * b.x;
            m._22 = a.y * b.y;
            m._23 = a.y * b.z;

            m._31 = a.z * b.x;
            m._32 = a.z * b.y;
            m._33 = a.z * b.z;

            return m;
        }

        float Matrix3::Trace() const
        {
            return _11 + _22 + _33;
        }

        Vector3 Matrix3::GetCol(unsigned int column) const
        {
            Vector3 out;
            MemoryCopy(&out, &values[3 * column], sizeof(Vector3));
            return out;
        }

        Vector3 Matrix3::GetRow(unsigned int row) const
        {
            return Vector3(values[row], values[row + 3], values[row + 6]);
        }

        void Matrix3::SetRow(unsigned int row, const Vector3& val)
        {
            values[row]     = val.x;
            values[row + 3] = val.y;
            values[row + 6] = val.z;
        }

        void Matrix3::SetCol(unsigned int column, const Vector3& val)
        {
            MemoryCopy(&values[column * 3], &val, sizeof(Vector3));
        }

        Vector3 Matrix3::GetDiagonal() const
        {
            return Vector3(values[0], values[4], values[8]);
        }

        void Matrix3::SetDiagonal(const Vector3& v)
        {
            values[0] = v.x;
            values[4] = v.y;
            values[8] = v.z;
        }

        Vector3 Matrix3::GetScalingVector() const
        {
            return Vector3(_11, _22, _33);
        }
        void Matrix3::SetScalingVector(const Vector3& in)
        {
            _11 = in.x;
            _22 = in.y;
            _33 = in.z;
        }
    }
}
