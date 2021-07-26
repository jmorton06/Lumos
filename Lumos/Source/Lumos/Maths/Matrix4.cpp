#include "Precompiled.h"

#include "Maths/Matrix4.h"
#include "Maths/Matrix4.h"

namespace Lumos::Maths
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

    bool Matrix4::IsDepthZeroOne()
    {
        return CONFIG_CLIP_CONTROL == CLIP_CONTROL_RH_ZO || CONFIG_CLIP_CONTROL == CLIP_CONTROL_LH_ZO;
    }

    const Matrix4 Matrix4::ZERO(
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f);

    const Matrix4 Matrix4::IDENTITY;

    void Matrix4::Decompose(Vector3& translation, Quaternion& rotation, Vector3& scale) const
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

    Matrix4 Matrix4::Inverse() const
    {
        float v0 = m20_ * m31_ - m21_ * m30_;
        float v1 = m20_ * m32_ - m22_ * m30_;
        float v2 = m20_ * m33_ - m23_ * m30_;
        float v3 = m21_ * m32_ - m22_ * m31_;
        float v4 = m21_ * m33_ - m23_ * m31_;
        float v5 = m22_ * m33_ - m23_ * m32_;

        float i00 = (v5 * m11_ - v4 * m12_ + v3 * m13_);
        float i10 = -(v5 * m10_ - v2 * m12_ + v1 * m13_);
        float i20 = (v4 * m10_ - v2 * m11_ + v0 * m13_);
        float i30 = -(v3 * m10_ - v1 * m11_ + v0 * m12_);

        float invDet = 1.0f / (i00 * m00_ + i10 * m01_ + i20 * m02_ + i30 * m03_);

        i00 *= invDet;
        i10 *= invDet;
        i20 *= invDet;
        i30 *= invDet;

        float i01 = -(v5 * m01_ - v4 * m02_ + v3 * m03_) * invDet;
        float i11 = (v5 * m00_ - v2 * m02_ + v1 * m03_) * invDet;
        float i21 = -(v4 * m00_ - v2 * m01_ + v0 * m03_) * invDet;
        float i31 = (v3 * m00_ - v1 * m01_ + v0 * m02_) * invDet;

        v0 = m10_ * m31_ - m11_ * m30_;
        v1 = m10_ * m32_ - m12_ * m30_;
        v2 = m10_ * m33_ - m13_ * m30_;
        v3 = m11_ * m32_ - m12_ * m31_;
        v4 = m11_ * m33_ - m13_ * m31_;
        v5 = m12_ * m33_ - m13_ * m32_;

        float i02 = (v5 * m01_ - v4 * m02_ + v3 * m03_) * invDet;
        float i12 = -(v5 * m00_ - v2 * m02_ + v1 * m03_) * invDet;
        float i22 = (v4 * m00_ - v2 * m01_ + v0 * m03_) * invDet;
        float int32_t = -(v3 * m00_ - v1 * m01_ + v0 * m02_) * invDet;

        v0 = m21_ * m10_ - m20_ * m11_;
        v1 = m22_ * m10_ - m20_ * m12_;
        v2 = m23_ * m10_ - m20_ * m13_;
        v3 = m22_ * m11_ - m21_ * m12_;
        v4 = m23_ * m11_ - m21_ * m13_;
        v5 = m23_ * m12_ - m22_ * m13_;

        float i03 = -(v5 * m01_ - v4 * m02_ + v3 * m03_) * invDet;
        float i13 = (v5 * m00_ - v2 * m02_ + v1 * m03_) * invDet;
        float i23 = -(v4 * m00_ - v2 * m01_ + v0 * m03_) * invDet;
        float i33 = (v3 * m00_ - v1 * m01_ + v0 * m02_) * invDet;

        return Matrix4(
            i00, i01, i02, i03,
            i10, i11, i12, i13,
            i20, i21, i22, i23,
            i30, i31, int32_t, i33);
    }

    Matrix4 PerspectiveRH_ZO(float znear, float zfar, float aspect, float fov, float offsetX = 0.0f, float offsetY = 0.0f, float zoom = 1.0f)
    {
        Matrix4 m;
        const float h = zoom / tan(fov * M_DEGTORAD * 0.5f);
        float neg_depth_r = 1.0f / (znear - zfar);

        m.m02_ = offsetX * 2.0f;
        m.m12_ = offsetY * 2.0f;

        m.m00_ = h / aspect;
        m.m11_ = -h;
        m.m22_ = (zfar)*neg_depth_r;
        m.m32_ = -1.0f;
        m.m23_ = (znear * zfar) * neg_depth_r;
        m.m33_ = 0.0f;

        return m;
    }

    Matrix4 PerspectiveRH_NO(float znear, float zfar, float aspect, float fov, float offsetX = 0.0f, float offsetY = 0.0f, float zoom = 1.0f)
    {
        Matrix4 m;
        const float h = zoom / tan(fov * M_DEGTORAD * 0.5f);
        float neg_depth_r = 1.0f / (znear - zfar);

        m.m02_ = offsetX * 2.0f;
        m.m12_ = offsetY * 2.0f;

        m.m00_ = h / aspect;
        m.m11_ = h;
        m.m22_ = (zfar + znear) * neg_depth_r;
        m.m32_ = -1.0f;
        m.m23_ = 2.0f * (znear * zfar) * neg_depth_r;
        m.m33_ = 0.0f;

        return m;
    }

    Matrix4 PerspectiveLH_ZO(float znear, float zfar, float aspect, float fov, float offsetX = 0.0f, float offsetY = 0.0f, float zoom = 1.0f)
    {
        Matrix4 m;
        const float h = zoom / tan(fov * M_DEGTORAD * 0.5f);
        float neg_depth_r = 1.0f / (znear - zfar);

        m.m02_ = offsetX * 2.0f;
        m.m12_ = offsetY * 2.0f;

        m.m00_ = h / aspect;
        m.m11_ = h;
        m.m22_ = (zfar)*neg_depth_r;
        m.m32_ = 1.0f;
        m.m23_ = (znear * zfar) * neg_depth_r;
        m.m33_ = 0.0f;

        return m;
    }

    Matrix4 PerspectiveLH_NO(float znear, float zfar, float aspect, float fov, float offsetX = 0.0f, float offsetY = 0.0f, float zoom = 1.0f)
    {
        Matrix4 m;
        const float h = zoom / tan(fov * M_DEGTORAD * 0.5f);
        float neg_depth_r = 1.0f / (zfar - zfar);

        m.m02_ = offsetX * 2.0f;
        m.m12_ = offsetY * 2.0f;

        m.m00_ = h / aspect;
        m.m11_ = h;
        m.m22_ = (zfar + znear) * neg_depth_r;
        m.m32_ = 1.0f;
        m.m23_ = -2.0f * (znear * zfar) * neg_depth_r;
        m.m33_ = 0.0f;

        return m;
    }

    Matrix4 Matrix4::Perspective(float znear, float zfar, float aspect, float fov, float offsetX, float offsetY, float zoom)
    {
        if(CONFIG_CLIP_CONTROL == CLIP_CONTROL_LH_ZO)
            return PerspectiveLH_ZO(znear, zfar, aspect, fov, offsetX, offsetY, zoom);
        else if(CONFIG_CLIP_CONTROL == CLIP_CONTROL_LH_NO)
            return PerspectiveLH_NO(znear, zfar, aspect, fov, offsetX, offsetY, zoom);
        else if(CONFIG_CLIP_CONTROL == CLIP_CONTROL_RH_ZO)
            return PerspectiveRH_ZO(znear, zfar, aspect, fov, offsetX, offsetY, zoom);
        else if(CONFIG_CLIP_CONTROL == CLIP_CONTROL_RH_NO)
            return PerspectiveRH_NO(znear, zfar, aspect, fov, offsetX, offsetY, zoom);

        return PerspectiveRH_NO(znear, zfar, aspect, fov, offsetX, offsetY, zoom);
    }

    Matrix4 OrthographicRH_ZO(float znear, float zfar, float right, float left, float top, float bottom, float offsetX = 0.0f, float offsetY = 0.0f, float zoom = 1.0f)
    {
        Matrix4 m;
        float x_r = 1.0f / (right - left);
        float y_r = zoom / (top - bottom);
        float z_r = 1.0f / (zfar - znear);

        m.m02_ = offsetX * 2.0f;
        m.m12_ = offsetY * 2.0f;

        m.m00_ = 2.0f * x_r;
        m.m11_ = -2.0f * y_r;
        m.m22_ = -1.0f * z_r;

        m.m03_ = -(right + left) * x_r;
        m.m13_ = -(top + bottom) * y_r;
        m.m23_ = -(znear)*z_r;
        m.m33_ = 1.0f;

        return m;
    }

    Matrix4 OrthographicRH_NO(float znear, float zfar, float right, float left, float top, float bottom, float offsetX = 0.0f, float offsetY = 0.0f, float zoom = 1.0f)
    {
        Matrix4 m;
        float x_r = 1.0f / (right - left);
        float y_r = zoom / (top - bottom);
        float z_r = 1.0f / (zfar - znear);

        m.m02_ = offsetX * 2.0f;
        m.m12_ = offsetY * 2.0f;

        m.m00_ = 2.0f * x_r;
        m.m11_ = 2.0f * y_r;
        m.m22_ = -2.0f * z_r;

        m.m03_ = -(right + left) * x_r;
        m.m13_ = -(top + bottom) * y_r;
        m.m23_ = -(zfar + znear) * z_r;
        m.m33_ = 1.0f;

        return m;
    }

    Matrix4 OrthographicLH_ZO(float znear, float zfar, float right, float left, float top, float bottom, float offsetX = 0.0f, float offsetY = 0.0f, float zoom = 1.0f)
    {
        Matrix4 m;
        float x_r = 1.0f / (right - left);
        float y_r = zoom / (top - bottom);
        float z_r = 1.0f / (zfar - znear);

        m.m02_ = offsetX * 2.0f;
        m.m12_ = offsetY * 2.0f;

        m.m00_ = 2.0f * x_r;
        m.m11_ = 2.0f * y_r;
        m.m22_ = -1.0f * z_r;

        m.m03_ = -(right + left) * x_r;
        m.m13_ = -(top + bottom) * y_r;
        m.m23_ = -(znear)*z_r;
        m.m33_ = 1.0f;

        return m;
    }

    Matrix4 OrthographicLH_NO(float znear, float zfar, float right, float left, float top, float bottom, float offsetX = 0.0f, float offsetY = 0.0f, float zoom = 1.0f)
    {
        Matrix4 m;
        float x_r = 1.0f / (right - left);
        float y_r = zoom / (top - bottom);
        float z_r = 1.0f / (zfar - znear);

        m.m02_ = offsetX * 2.0f;
        m.m12_ = offsetY * 2.0f;

        m.m00_ = 2.0f * x_r;
        m.m11_ = 2.0f * y_r;
        m.m22_ = 2.0f * z_r;

        m.m03_ = -(right + left) * x_r;
        m.m13_ = -(top + bottom) * y_r;
        m.m23_ = -(zfar + znear) * z_r;
        m.m33_ = 1.0f;

        return m;
    }

    Matrix4 Matrix4::Orthographic(float left, float right, float bottom, float top, float znear, float zfar, float offsetX, float offsetY, float zoom)
    {
        if(CONFIG_CLIP_CONTROL == CLIP_CONTROL_LH_ZO)
            return OrthographicLH_ZO(znear, zfar, right, left, top, bottom, offsetX, offsetY, zoom);
        else if(CONFIG_CLIP_CONTROL == CLIP_CONTROL_LH_NO)
            return OrthographicLH_NO(znear, zfar, right, left, top, bottom, offsetX, offsetY, zoom);
        else if(CONFIG_CLIP_CONTROL == CLIP_CONTROL_RH_ZO)
            return OrthographicRH_ZO(znear, zfar, right, left, top, bottom, offsetX, offsetY, zoom);
        else if(CONFIG_CLIP_CONTROL == CLIP_CONTROL_RH_NO)
            return OrthographicRH_NO(znear, zfar, right, left, top, bottom, offsetX, offsetY, zoom);

        return OrthographicRH_NO(znear, zfar, right, left, top, bottom, offsetX, offsetY, zoom);
    }
}
