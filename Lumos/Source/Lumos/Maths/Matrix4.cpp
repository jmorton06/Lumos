#include "Precompiled.h"
#include "Matrix4.h"
#include "Matrix3.h"
#include "Quaternion.h"
#include "MathsUtilities.h"
#include "SSEUtilities.h"

namespace Lumos
{
    namespace Maths
    {
        int Matrix4::CONFIG_CLIP_CONTROL = CLIP_CONTROL_RH_NO;

        void Matrix4::SetUpCoordSystem(bool LeftHanded, bool forceZeroToOne)
        {
            if(forceZeroToOne)
            {
                if(LeftHanded)
                {
                    CONFIG_CLIP_CONTROL = CLIP_CONTROL_LH_ZO;
                }
                else
                {
                    CONFIG_CLIP_CONTROL = CLIP_CONTROL_RH_ZO;
                }
            }
            else
            {
                if(LeftHanded)
                {
                    CONFIG_CLIP_CONTROL = CLIP_CONTROL_LH_NO;
                }
                else
                {
                    CONFIG_CLIP_CONTROL = CLIP_CONTROL_RH_NO;
                }
            }
        }

        Matrix4::Matrix4(const Matrix3& mat)
        {
            const unsigned int size = 3 * sizeof(float);
            memcpy(&values[0], &mat.values[0], size);
            memcpy(&values[4], &mat.values[3], size);
            memcpy(&values[8], &mat.values[6], size);
            values[3] = values[7] = values[12] = values[13] = values[14] = 0.0f;
            values[15]                                                   = 1.0f;
        }

        Matrix4::Matrix4(const Quaternion& quat)
        {
            *this = quat.ToMatrix4();
        }

        Matrix4::Matrix4(const Matrix4& mat)
        {
            memcpy(&values[0], &mat.values[0], sizeof(Matrix4));
        }

        Matrix4::Matrix4(const float elements[16])
        {
            memcpy(values, elements, 16 * sizeof(float));
        }

        void Matrix4::ToIdentity()
        {
#ifdef LUMOS_SSE
            mmvalues[0] = _mm_set_ps(0, 0, 0, 1);
            mmvalues[1] = _mm_set_ps(0, 0, 1, 0);
            mmvalues[2] = _mm_set_ps(0, 1, 0, 0);
            mmvalues[3] = _mm_set_ps(1, 0, 0, 0);
#else
            ToZero();
            values[0]  = 1.0f;
            values[5]  = 1.0f;
            values[10] = 1.0f;
            values[15] = 1.0f;
#endif
        }

        void Matrix4::ToZero()
        {
#ifdef LUMOS_SSE
            mmvalues[0] = mmvalues[1] = mmvalues[2] = mmvalues[3] = _mm_setzero_ps();
#else
            for(int i = 0; i < 16; i++)
            {
                values[i] = 0.0f;
            }
#endif
        }

        Matrix4 Matrix4::Transpose() const
        {
            Mat4 transposedMat4 = *this;
#ifdef LUMOS_SSE
            _MM_TRANSPOSE4_PS(transposedMat4.mmvalues[0], transposedMat4.mmvalues[1], transposedMat4.mmvalues[2], transposedMat4.mmvalues[3]);
#else
            float tmp[16];

            tmp[0] = values[0];
            tmp[1] = values[4];
            tmp[2] = values[8];
            tmp[3] = values[12];

            tmp[4] = values[1];
            tmp[5] = values[5];
            tmp[6] = values[9];
            tmp[7] = values[13];

            tmp[8]  = values[2];
            tmp[9]  = values[6];
            tmp[10] = values[10];
            tmp[11] = values[14];

            tmp[12] = values[3];
            tmp[13] = values[7];
            tmp[14] = values[11];
            tmp[15] = values[15];

            for(int i = 0; i < 16; ++i)
            {
                transposedMat4.values[i] = tmp[i];
            }
#endif
            return transposedMat4;
        }

        Matrix4 Matrix4::GetRotation() const
        {
            Vector3 invScale(
                1.0f / sqrtf(values[0] * values[0] + values[4] * values[4] + values[8] * values[8]),
                1.0f / sqrtf(values[1] * values[1] + values[5] * values[5] + values[9] * values[9]),
                1.0f / sqrtf(values[2] * values[2] + values[6] * values[6] + values[10] * values[10]));

            Matrix4 temp;
            temp.values[0]  = values[0] * invScale.x;
            temp.values[1]  = values[1] * invScale.y;
            temp.values[2]  = values[2] * invScale.z;
            temp.values[4]  = values[4] * invScale.x;
            temp.values[5]  = values[5] * invScale.y;
            temp.values[6]  = values[6] * invScale.z;
            temp.values[8]  = values[8] * invScale.x;
            temp.values[9]  = values[9] * invScale.y;
            temp.values[10] = values[10] * invScale.z;
            return temp;
        }

        Vector3 Matrix4::GetEulerAngles(const Matrix4& mat)
        {
            float angle_x = 0.0f;
            float angle_y = -asin(mat.values[8]);
            float angle_z = 0.0f;

            float c = cos(angle_y);
            angle_y = Lumos::Maths::ToDegrees(angle_y);

            if(fabs(c) > 0.005f)
            {
                c          = 1.0f / c;
                float tr_x = mat.values[10] * c;
                float tr_y = -mat.values[9] * c;

                angle_x = Lumos::Maths::ToDegrees(atan2(tr_y, tr_x));

                tr_x = mat.values[0] * c;
                tr_y = -mat.values[4] * c;

                angle_z = Lumos::Maths::ToDegrees(atan2(tr_y, tr_x));
            }
            else
            {
                float tr_x = mat.values[5];
                float tr_y = mat.values[1];

                angle_z = atan2(tr_y, tr_x);
            }

            return Vector3(Lumos::Maths::Clamp(angle_x, 0.0f, 360.0f), Lumos::Maths::Clamp(angle_y, 0.0f, 360.0f),
                           Lumos::Maths::Clamp(angle_z, 0.0f, 360.0f));
        }

        Vector4 Matrix4::GetRow(unsigned int row) const
        {
            return Vector4(values[row + 0 * 4], values[row + 1 * 4], values[row + 2 * 4], values[row + 3 * 4]);
        }

        Vector4 Matrix4::GetCol(unsigned int column) const
        {
            return Vector4(values[0 + column * 4], values[1 + column * 4], values[2 + column * 4], values[3 + column * 4]);
        }

        void Matrix4::SetRow(unsigned int row, const Vector4& val)
        {
            values[row]      = val.x;
            values[row + 4]  = val.y;
            values[row + 8]  = val.z;
            values[row + 12] = val.w;

            values[row + 0 * 4] = val.x;
            values[row + 1 * 4] = val.y;
            values[row + 2 * 4] = val.z;
            values[row + 3 * 4] = val.w;
        }

        void Matrix4::SetCol(unsigned int column, const Vector4& val)
        {
#if defined(LUMOS_SSE)
            mmvalues[column] = val.m_Value;
#else
            values[0 + column * 4] = val.x;
            values[1 + column * 4] = val.y;
            values[2 + column * 4] = val.z;
            values[3 + column * 4] = val.w;
#endif
        }

        Vector3 Matrix4::GetPositionVector() const
        {
            return Vector3(values[12], values[13], values[14]);
        }

#if defined(LUMOS_SSE)
        Vector3 Matrix4::GetTranslationMemAligned() const
        {
            return Vector3(mmvalues[3]);
        }
#endif

        void Matrix4::SetPositionVector(const Vector3& v)
        {
            values[12] = v.x;
            values[13] = v.y;
            values[14] = v.z;
        }

        Vector3 Matrix4::GetScaling() const
        {
            return Vector3(
                sqrtf(values[0] * values[0] + values[4] * values[4] + values[8] * values[8]),
                sqrtf(values[1] * values[1] + values[5] * values[5] + values[9] * values[9]),
                sqrtf(values[2] * values[2] + values[6] * values[6] + values[10] * values[10]));
        }

        void Matrix4::SetScaling(const Vector3& in)
        {
            values[0]  = in.x;
            values[5]  = in.y;
            values[10] = in.z;
        }

        Quaternion Matrix4::ToQuaternion() const
        {
            Mat4 test = GetRotation();
            return Quaternion::FromMatrix(test);
        }

        Vector3 Matrix4::Translation() const
        {
            return Vector3(values[12], values[13], values[14]);
        }

        Quaternion Matrix4::Rotation() const
        {
            return Quaternion::FromMatrix(GetRotation());
        }

        Vector3 Matrix4::Scale() const
        {
            return Vector3(
                sqrt(values[0] * values[0] + values[1] * values[1] + values[2] * values[2]),
                sqrt(values[4] * values[4] + values[5] * values[5] + values[6] * values[6]),
                sqrt(values[8] * values[8] + values[9] * values[9] + values[10] * values[10]));
        }

        void Matrix4::Decompose(Vector3& translation, Quaternion& rotation, Vector3& scale) const
        {
            translation = Translation();
            scale       = Scale();
            rotation    = ToQuaternion();
        }

        Matrix4 Matrix4::Inverse(const Matrix4& inM)
        {
#ifdef LUMOS_SSE
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
            tr        = _mm_hadd_ps(tr, tr);
            tr        = _mm_hadd_ps(tr, tr);
            // |M| = |A|*|D| + |B|*|C| - tr((A#B)(D#C)
            detM = _mm_sub_ps(detM, tr);

            float detScalar = _mm_cvtss_f32(detM);
            if(Maths::Equals(detScalar, 0.0f))
            {
                Matrix4 identity;
                identity.ToIdentity();
                return identity;
            }

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

            Matrix4 inv(1.0f);

            inv[0] = inM[5] * inM[10] * inM[15] - inM[5] * inM[11] * inM[14] - inM[9] * inM[6] * inM[15] + inM[9] * inM[7] * inM[14] + inM[13] * inM[6] * inM[11] - inM[13] * inM[7] * inM[10];

            inv[4] = -inM[4] * inM[10] * inM[15] + inM[4] * inM[11] * inM[14] + inM[8] * inM[6] * inM[15] - inM[8] * inM[7] * inM[14] - inM[12] * inM[6] * inM[11] + inM[12] * inM[7] * inM[10];

            inv[8] = inM[4] * inM[9] * inM[15] - inM[4] * inM[11] * inM[13] - inM[8] * inM[5] * inM[15] + inM[8] * inM[7] * inM[13] + inM[12] * inM[5] * inM[11] - inM[12] * inM[7] * inM[9];

            inv[12] = -inM[4] * inM[9] * inM[14] + inM[4] * inM[10] * inM[13] + inM[8] * inM[5] * inM[14] - inM[8] * inM[6] * inM[13] - inM[12] * inM[5] * inM[10] + inM[12] * inM[6] * inM[9];

            inv[1] = -inM[1] * inM[10] * inM[15] + inM[1] * inM[11] * inM[14] + inM[9] * inM[2] * inM[15] - inM[9] * inM[3] * inM[14] - inM[13] * inM[2] * inM[11] + inM[13] * inM[3] * inM[10];

            inv[5] = inM[0] * inM[10] * inM[15] - inM[0] * inM[11] * inM[14] - inM[8] * inM[2] * inM[15] + inM[8] * inM[3] * inM[14] + inM[12] * inM[2] * inM[11] - inM[12] * inM[3] * inM[10];

            inv[9] = -inM[0] * inM[9] * inM[15] + inM[0] * inM[11] * inM[13] + inM[8] * inM[1] * inM[15] - inM[8] * inM[3] * inM[13] - inM[12] * inM[1] * inM[11] + inM[12] * inM[3] * inM[9];

            inv[13] = inM[0] * inM[9] * inM[14] - inM[0] * inM[10] * inM[13] - inM[8] * inM[1] * inM[14] + inM[8] * inM[2] * inM[13] + inM[12] * inM[1] * inM[10] - inM[12] * inM[2] * inM[9];

            inv[2] = inM[1] * inM[6] * inM[15] - inM[1] * inM[7] * inM[14] - inM[5] * inM[2] * inM[15] + inM[5] * inM[3] * inM[14] + inM[13] * inM[2] * inM[7] - inM[13] * inM[3] * inM[6];

            inv[6] = -inM[0] * inM[6] * inM[15] + inM[0] * inM[7] * inM[14] + inM[4] * inM[2] * inM[15] - inM[4] * inM[3] * inM[14] - inM[12] * inM[2] * inM[7] + inM[12] * inM[3] * inM[6];

            inv[10] = inM[0] * inM[5] * inM[15] - inM[0] * inM[7] * inM[13] - inM[4] * inM[1] * inM[15] + inM[4] * inM[3] * inM[13] + inM[12] * inM[1] * inM[7] - inM[12] * inM[3] * inM[5];

            inv[14] = -inM[0] * inM[5] * inM[14] + inM[0] * inM[6] * inM[13] + inM[4] * inM[1] * inM[14] - inM[4] * inM[2] * inM[13] - inM[12] * inM[1] * inM[6] + inM[12] * inM[2] * inM[5];

            inv[3] = -inM[1] * inM[6] * inM[11] + inM[1] * inM[7] * inM[10] + inM[5] * inM[2] * inM[11] - inM[5] * inM[3] * inM[10] - inM[9] * inM[2] * inM[7] + inM[9] * inM[3] * inM[6];

            inv[7] = inM[0] * inM[6] * inM[11] - inM[0] * inM[7] * inM[10] - inM[4] * inM[2] * inM[11] + inM[4] * inM[3] * inM[10] + inM[8] * inM[2] * inM[7] - inM[8] * inM[3] * inM[6];

            inv[11] = -inM[0] * inM[5] * inM[11] + inM[0] * inM[7] * inM[9] + inM[4] * inM[1] * inM[11] - inM[4] * inM[3] * inM[9] - inM[8] * inM[1] * inM[7] + inM[8] * inM[3] * inM[5];

            inv[15] = inM[0] * inM[5] * inM[10] - inM[0] * inM[6] * inM[9] - inM[4] * inM[1] * inM[10] + inM[4] * inM[2] * inM[9] + inM[8] * inM[1] * inM[6] - inM[8] * inM[2] * inM[5];

            float det = inM[0] * inv[0] + inM[1] * inv[4] + inM[2] * inv[8] + inM[3] * inv[12];

            if(Maths::Equals(det, 0.0f))
            {
                inv.ToIdentity();
                return inv;
            }

            det = 1.f / det;

            for(int i = 0; i < 16; i++)
            {
                inv[i] = inv[i] * det;
            }

            return inv;

#endif
        }

        Matrix4 PerspectiveRH_ZO(float znear, float zfar, float aspect, float fov)
        {
            Matrix4 m(0.0f);
            const float h     = 1.0f / tan(fov * Lumos::Maths::M_DEGTORAD_2);
            float neg_depth_r = 1.0f / (znear - zfar);

            m.values[0]  = h / aspect;
            m.values[5]  = h;
            m.values[10] = (zfar)*neg_depth_r;
            m.values[11] = -1.0f;
            m.values[14] = (znear * zfar) * neg_depth_r;
            m.values[15] = 0.0f;

            return m;
        }

        Matrix4 PerspectiveRH_NO(float znear, float zfar, float aspect, float fov)
        {
            Matrix4 m(0.0f);
            const float h     = 1.0f / tan(fov * Lumos::Maths::M_DEGTORAD_2);
            float neg_depth_r = 1.0f / (znear - zfar);

            m.values[0]  = h / aspect;
            m.values[5]  = h;
            m.values[10] = (zfar + znear) * neg_depth_r;
            m.values[11] = -1.0f;
            m.values[14] = 2.0f * (znear * zfar) * neg_depth_r;
            m.values[15] = 0.0f;

            return m;
        }

        Matrix4 PerspectiveLH_ZO(float znear, float zfar, float aspect, float fov)
        {
            Matrix4 m(0.0f);
            const float h     = 1.0f / tan(fov * Lumos::Maths::M_DEGTORAD_2);
            float neg_depth_r = 1.0f / (znear - zfar);

            m.values[0]  = h / aspect;
            m.values[5]  = h;
            m.values[10] = (zfar)*neg_depth_r;
            m.values[11] = 1.0f;
            m.values[14] = (znear * zfar) * neg_depth_r;
            m.values[15] = 0.0f;

            return m;
        }

        Matrix4 PerspectiveLH_NO(float znear, float zfar, float aspect, float fov)
        {
            Matrix4 m(0.0f);
            const float h     = 1.0f / tan(fov * Lumos::Maths::M_DEGTORAD_2);
            float neg_depth_r = 1.0f / (znear - zfar);

            m.values[0]  = h / aspect;
            m.values[5]  = h;
            m.values[10] = (zfar + znear) * neg_depth_r;
            m.values[11] = 1.0f;
            m.values[14] = 2.0f * (znear * zfar) * neg_depth_r;
            m.values[15] = 0.0f;

            return m;
        }

        Matrix4 Matrix4::Perspective(float znear, float zfar, float aspect, float fov)
        {
            if(CONFIG_CLIP_CONTROL == CLIP_CONTROL_LH_ZO)
                return PerspectiveLH_ZO(znear, zfar, aspect, fov);
            else if(CONFIG_CLIP_CONTROL == CLIP_CONTROL_LH_NO)
                return PerspectiveLH_NO(znear, zfar, aspect, fov);
            else if(CONFIG_CLIP_CONTROL == CLIP_CONTROL_RH_ZO)
                return PerspectiveRH_ZO(znear, zfar, aspect, fov);
            else if(CONFIG_CLIP_CONTROL == CLIP_CONTROL_RH_NO)
                return PerspectiveRH_NO(znear, zfar, aspect, fov);

            return PerspectiveRH_NO(znear, zfar, aspect, fov);
        }

        Matrix4 OrthographicRH_ZO(float znear, float zfar, float right, float left, float top, float bottom)
        {
            Matrix4 m;
            float x_r = 1.0f / (right - left);
            float y_r = 1.0f / (top - bottom);
            float z_r = 1.0f / (zfar - znear);

            m.values[0]  = 2.0f * x_r;
            m.values[5]  = 2.0f * y_r;
            m.values[10] = -1.0f * z_r;

            m.values[12] = -(right + left) * x_r;
            m.values[13] = -(top + bottom) * y_r;
            m.values[14] = -(znear)*z_r;
            m.values[15] = 1.0f;

            return m;
        }

        Matrix4 OrthographicRH_NO(float znear, float zfar, float right, float left, float top, float bottom)
        {
            Matrix4 m;
            float x_r = 1.0f / (right - left);
            float y_r = 1.0f / (top - bottom);
            float z_r = 1.0f / (zfar - znear);

            m.values[0]  = 2.0f * x_r;
            m.values[5]  = 2.0f * y_r;
            m.values[10] = -2.0f * z_r;

            m.values[12] = -(right + left) * x_r;
            m.values[13] = -(top + bottom) * y_r;
            m.values[14] = -(zfar + znear) * z_r;
            m.values[15] = 1.0f;

            return m;
        }

        Matrix4 OrthographicLH_ZO(float znear, float zfar, float right, float left, float top, float bottom)
        {
            Matrix4 m;
            float x_r = 1.0f / (right - left);
            float y_r = 1.0f / (top - bottom);
            float z_r = 1.0f / (zfar - znear);

            m.values[0]  = 2.0f * x_r;
            m.values[5]  = 2.0f * y_r;
            m.values[10] = -1.0f * z_r;

            m.values[12] = -(right + left) * x_r;
            m.values[13] = -(top + bottom) * y_r;
            m.values[14] = -(znear)*z_r;
            m.values[15] = 1.0f;

            return m;
        }

        Matrix4 OrthographicLH_NO(float znear, float zfar, float right, float left, float top, float bottom)
        {
            Matrix4 m;
            float x_r = 1.0f / (right - left);
            float y_r = 1.0f / (top - bottom);
            float z_r = 1.0f / (zfar - znear);

            m.values[0]  = 2.0f * x_r;
            m.values[5]  = 2.0f * y_r;
            m.values[10] = -1.0f * z_r;

            m.values[12] = -(right + left) * x_r;
            m.values[13] = -(top + bottom) * y_r;
            m.values[14] = -(znear)*z_r;
            m.values[15] = 1.0f;

            return m;
        }

        Matrix4 Matrix4::Orthographic(float left, float right, float bottom, float top, float znear, float zfar)
        {
            if(CONFIG_CLIP_CONTROL == CLIP_CONTROL_LH_ZO)
                return OrthographicLH_ZO(znear, zfar, right, left, top, bottom);
            else if(CONFIG_CLIP_CONTROL == CLIP_CONTROL_LH_NO)
                return OrthographicLH_NO(znear, zfar, right, left, top, bottom);
            else if(CONFIG_CLIP_CONTROL == CLIP_CONTROL_RH_ZO)
                return OrthographicRH_ZO(znear, zfar, right, left, top, bottom);
            else if(CONFIG_CLIP_CONTROL == CLIP_CONTROL_RH_NO)
                return OrthographicRH_NO(znear, zfar, right, left, top, bottom);

            return OrthographicRH_NO(znear, zfar, right, left, top, bottom);
        }

        Matrix4 Matrix4::LookAt(const Vector3& from, const Vector3& lookingAt, const Vector3& up)
        {
            // RH

            Matrix4 Result(1.0f);
            Vector3 f = (lookingAt - from);
            f.Normalise();

            Vector3 s = Vector3::Cross(f, up);
            s.Normalise();

            Vector3 u = Vector3::Cross(s, f);

            Result[0] = s.x;
            Result[4] = s.y;
            Result[8] = s.z;

            Result[1] = u.x;
            Result[5] = u.y;
            Result[9] = u.z;

            Result[2]  = -f.x;
            Result[6]  = -f.y;
            Result[10] = -f.z;

            Result[12] = -Maths::Dot(s, from);
            Result[13] = -Maths::Dot(u, from);
            Result[14] = Maths::Dot(f, from);
            return Result;
        }

        Matrix4 Matrix4::RotationX(float degrees)
        {
            Matrix4 m(1.0f);
            float rad = Lumos::Maths::ToRadians(degrees);
            float c   = cos(rad);
            float s   = sin(rad);

            m.values[5] = c;
            m.values[6] = s;

            m.values[9]  = -s;
            m.values[10] = c;

            return m;
        }

        Matrix4 Matrix4::RotationY(float degrees)
        {
            Matrix4 m(1.0f);
            float rad = Lumos::Maths::ToRadians(degrees);
            float c   = cos(rad);
            float s   = sin(rad);

            m.values[0] = c;
            m.values[2] = s;

            m.values[8]  = -s;
            m.values[10] = c;

            return m;
        }

        Matrix4 Matrix4::RotationZ(float degrees)
        {
            Matrix4 m(1.0f);
            float rad = Lumos::Maths::ToRadians(degrees);
            float c   = cos(rad);
            float s   = sin(rad);

            m.values[0] = c;
            m.values[1] = -s;

            m.values[4] = s;
            m.values[5] = c;

            return m;
        }

        Matrix4 Matrix4::Rotation(float degrees, const Vector3& axis)
        {
#ifdef LUMOS_SSE
            Vector3 axisNorm = axis;
            axisNorm.Normalise();

            float rad = Lumos::Maths::ToRadians(degrees);
            float c   = cos(rad);
            float s   = sin(rad);

            __m128 normXYZW      = _mm_set_ps(0, axisNorm.z, axisNorm.y, axisNorm.x);
            __m128 normXYZWWithC = _mm_mul_ps(normXYZW, _mm_set1_ps(1.0f - c));
            __m128 col0          = _mm_mul_ps(normXYZW, _mm_set1_ps(GetValue(normXYZWWithC, 0)));
            __m128 col1          = _mm_mul_ps(normXYZW, _mm_set1_ps(GetValue(normXYZWWithC, 1)));
            __m128 col2          = _mm_mul_ps(normXYZW, _mm_set1_ps(GetValue(normXYZWWithC, 2)));

            Matrix4 m;
            m.values[0]  = GetValue(col0, 0) + c;
            m.values[1]  = GetValue(col0, 1) + (axisNorm.z * s);
            m.values[2]  = GetValue(col0, 2) - (axisNorm.y * s);
            m.values[4]  = GetValue(col1, 0) - (axisNorm.z * s);
            m.values[5]  = GetValue(col1, 1) + c;
            m.values[6]  = GetValue(col1, 2) + (axisNorm.x * s);
            m.values[8]  = GetValue(col2, 0) + (axisNorm.y * s);
            m.values[9]  = GetValue(col2, 1) - (axisNorm.x * s);
            m.values[10] = GetValue(col2, 2) + c;

            return m;

#else
            Matrix4 m(1.0f);

            Vector3 axisNorm = axis;
            axisNorm.Normalise();

            float c = cos(static_cast<float>(Maths::ToRadians(degrees)));
            float s = sin(static_cast<float>(Maths::ToRadians(degrees)));

            m.values[0] = (axisNorm.x * axisNorm.x) * (1.0f - c) + c;
            m.values[1] = (axisNorm.y * axisNorm.x) * (1.0f - c) + (axisNorm.z * s);
            m.values[2] = (axisNorm.z * axisNorm.x) * (1.0f - c) - (axisNorm.y * s);

            m.values[4] = (axisNorm.x * axisNorm.y) * (1.0f - c) - (axisNorm.z * s);
            m.values[5] = (axisNorm.y * axisNorm.y) * (1.0f - c) + c;
            m.values[6] = (axisNorm.z * axisNorm.y) * (1.0f - c) + (axisNorm.x * s);

            m.values[8]  = (axisNorm.x * axisNorm.z) * (1.0f - c) + (axisNorm.y * s);
            m.values[9]  = (axisNorm.y * axisNorm.z) * (1.0f - c) - (axisNorm.x * s);
            m.values[10] = (axisNorm.z * axisNorm.z) * (1.0f - c) + c;

            return m;
#endif
        }

        Matrix4 Matrix4::Rotation(float degreesX, float degreesY, float degreesZ)
        {
            // Building this matrix directly is faster than multiplying three matrices for X, Y and Z
            float phi = Lumos::Maths::ToRadians(degreesX), theta = Lumos::Maths::ToRadians(degreesY), psi = Lumos::Maths::ToRadians(degreesZ);
            float sinTh = sin(theta), cosTh = cos(theta),
                  sinPh = sin(phi), cosPh = cos(phi),
                  sinPs = sin(psi), cosPs = cos(psi);

            Matrix4 result;
            result.values[0]  = cosTh * cosPs;
            result.values[1]  = cosTh * sinPs;
            result.values[2]  = -sinTh;
            result.values[4]  = -cosPh * sinPs + sinPh * sinTh * cosPs;
            result.values[5]  = cosPh * cosPs + sinPh * sinTh * sinPs;
            result.values[6]  = sinPh * cosTh;
            result.values[8]  = sinPh * sinPs + cosPh * sinTh * cosPs;
            result.values[9]  = -sinPh * cosPs + cosPh * sinTh * sinPs;
            result.values[10] = cosPh * cosTh;
            return result;
        }

        Matrix4 Matrix4::Scale(const Vector3& scale)
        {
            Matrix4 m(1.0f);
            m.values[0]  = scale.x;
            m.values[5]  = scale.y;
            m.values[10] = scale.z;
            return m;
        }

        void Matrix4::SetTranslation(const Vector3& translation)
        {
            values[12] = translation.x;
            values[13] = translation.y;
            values[14] = translation.z;
        }

        Matrix4 Matrix4::Translation(const Vector3& translation)
        {
            Matrix4 m(1.0f);
            m.values[12] = translation.x;
            m.values[13] = translation.y;
            m.values[14] = translation.z;
            return m;
        }

        Matrix4 Matrix4::operator*(const Matrix4& m) const
        {
#ifdef LUMOS_SSE
            // http://fhtr.blogspot.co.uk/2010/02/4x4-float-matrix-multiplication-using.html
            Matrix4 result;
            result.ToZero();
            for(u32 i = 0; i < 4; i++)
                for(u32 j = 0; j < 4; j++)
                    result.mmvalues[j] = _mm_add_ps(_mm_mul_ps(_mm_set1_ps(m.values[j * 4 + i]), mmvalues[i]),
                                                    result.mmvalues[j]);
            return result;
#else
            Matrix4 out;

            for(u32 r = 0; r < 4; r++)
            {
                for(u32 c = 0; c < 4; c++)
                {
                    float sum = 0.0f;
                    for(u32 i = 0; i < 4; i++)
                    {
                        sum += values[c + (i * 4)] * m.values[(r * 4) + i];
                    }
                    out.values[c + (r * 4)] = sum;
                }
            }
            return out;
#endif
        }

        Vector3 Matrix4::operator*(const Vector3& v) const
        {
#ifdef LUMOS_SSE_VEC3
            // Breaks collision - bounding box
            __m128 m0 = _mm_mul_ps(mmvalues[0], _mm_set1_ps(v.x));
            __m128 m1 = _mm_mul_ps(mmvalues[1], _mm_set1_ps(v.y));
            __m128 m2 = _mm_mul_ps(mmvalues[2], _mm_set1_ps(v.z));

            __m128 tempValue = _mm_add_ps(_mm_add_ps(m0, m1), _mm_add_ps(m2, mmvalues[3]));
            return _mm_div_ps(tempValue, _mm_shuffle_ps(tempValue, tempValue, _MM_SHUFFLE(0, 0, 0, 0)));

#else
            Vector3 vec;

            float temp;

            vec.x = v.x * values[0] + v.y * values[4] + v.z * values[8] + values[12];
            vec.y = v.x * values[1] + v.y * values[5] + v.z * values[9] + values[13];
            vec.z = v.x * values[2] + v.y * values[6] + v.z * values[10] + values[14];

            temp = v.x * values[3] + v.y * values[7] + v.z * values[11] + values[15];

            vec.x = vec.x / temp;
            vec.y = vec.y / temp;
            vec.z = vec.z / temp;

            return vec;
#endif
        };

        Vector4 Matrix4::operator*(const Vector4& v) const
        {
#ifdef LUMOS_SSE
            __m128 m0 = _mm_mul_ps(mmvalues[0], _mm_set1_ps(v.x));
            __m128 m1 = _mm_mul_ps(mmvalues[1], _mm_set1_ps(v.y));
            __m128 m2 = _mm_mul_ps(mmvalues[2], _mm_set1_ps(v.z));
            __m128 m3 = _mm_mul_ps(mmvalues[3], _mm_set1_ps(v.w));

            return Vector4(_mm_add_ps(_mm_add_ps(m0, m1), _mm_add_ps(m2, m3)));
#else
            return Vector4(v.x * values[0] + v.y * values[4] + v.z * values[8] + v.w * values[12],
                           v.x * values[1] + v.y * values[5] + v.z * values[9] + v.w * values[13],
                           v.x * values[2] + v.y * values[6] + v.z * values[10] + v.w * values[14],
                           v.x * values[3] + v.y * values[7] + v.z * values[11] + v.w * values[15]);
#endif
        }

        Matrix4 Matrix4::operator*(float rhs) const
        {
            Matrix4 result;
            result.ToZero();
            for(int i = 0; i < 16; ++i)
                result.values[i] = values[i] * rhs;
            return result;
        };

        bool Matrix4::operator==(const Matrix4& m) const
        {
            for(int i = 0; i < 16; i++)
            {
                if(!Equals(values[i], m[i]))
                    return false;
            }
            return true;
        };
    }
}
