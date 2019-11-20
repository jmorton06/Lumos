#include "lmpch.h"

#include "Maths/Matrix3x4.h"

namespace Lumos::Maths
{

const Matrix3x4 Matrix3x4::ZERO(
    0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f);

const Matrix3x4 Matrix3x4::IDENTITY;

void Matrix3x4::Decompose(Vector3& translation, Quaternion& rotation, Vector3& scale) const
{
    translation.x = m03_;
    translation.y = m13_;
    translation.z = m23_;

    scale.x = sqrtf(m00_ * m00_ + m10_ * m10_ + m20_ * m20_);
    scale.y = sqrtf(m01_ * m01_ + m11_ * m11_ + m21_ * m21_);
    scale.z = sqrtf(m02_ * m02_ + m12_ * m12_ + m22_ * m22_);

    Vector3 invScale(1.0f / scale.x, 1.0f / scale.y, 1.0f / scale.z);
    rotation = Quaternion(ToMatrix3().Scaled(invScale));
}

Matrix3x4 Matrix3x4::Inverse() const
{
    float det = m00_ * m11_ * m22_ +
                m10_ * m21_ * m02_ +
                m20_ * m01_ * m12_ -
                m20_ * m11_ * m02_ -
                m10_ * m01_ * m22_ -
                m00_ * m21_ * m12_;

    float invDet = 1.0f / det;
    Matrix3x4 ret;

    ret.m00_ = (m11_ * m22_ - m21_ * m12_) * invDet;
    ret.m01_ = -(m01_ * m22_ - m21_ * m02_) * invDet;
    ret.m02_ = (m01_ * m12_ - m11_ * m02_) * invDet;
    ret.m03_ = -(m03_ * ret.m00_ + m13_ * ret.m01_ + m23_ * ret.m02_);
    ret.m10_ = -(m10_ * m22_ - m20_ * m12_) * invDet;
    ret.m11_ = (m00_ * m22_ - m20_ * m02_) * invDet;
    ret.m12_ = -(m00_ * m12_ - m10_ * m02_) * invDet;
    ret.m13_ = -(m03_ * ret.m10_ + m13_ * ret.m11_ + m23_ * ret.m12_);
    ret.m20_ = (m10_ * m21_ - m20_ * m11_) * invDet;
    ret.m21_ = -(m00_ * m21_ - m20_ * m01_) * invDet;
    ret.m22_ = (m00_ * m11_ - m10_ * m01_) * invDet;
    ret.m23_ = -(m03_ * ret.m20_ + m13_ * ret.m21_ + m23_ * ret.m22_);

    return ret;
}

}
