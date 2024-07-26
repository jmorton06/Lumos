#include "Precompiled.h"
#include "Camera.h"

namespace Lumos
{
    Camera::Camera()
        : m_AspectRatio(1.0f)
        , m_Near(0.1f)
        , m_Far(100.0f)
        , m_Fov(60.0f)
        , m_ProjectionDirty(true)
        , m_FrustumDirty(true)
        , m_Orthographic(false)
        , m_MouseSensitivity(0.1f)
    {
    }

    Camera::Camera(float FOV, float Near, float Far, float aspect)
        : m_AspectRatio(aspect)
        , m_FrustumDirty(true)
        , m_ProjectionDirty(true)
        , m_Fov(FOV)
        , m_Near(Near)
        , m_Far(Far)
        , m_Orthographic(false)
        , m_Scale(1.0f) {};

    Camera::Camera(float pitch, float yaw, const Vec3& position, float FOV, float Near, float Far, float aspect)
        : m_AspectRatio(aspect)
        , m_FrustumDirty(true)
        , m_ProjectionDirty(true)
        , m_Fov(FOV)
        , m_Near(Near)
        , m_Far(Far)
        , m_Orthographic(false)
        , m_Scale(1.0f)
    {
    }

    Camera::Camera(float aspectRatio, float scale)
        : m_AspectRatio(aspectRatio)
        , m_Scale(scale)
        , m_FrustumDirty(true)
        , m_ProjectionDirty(true)
        , m_Fov(60)
        , m_Near(-10.0)
        , m_Far(10.0f)
        , m_Orthographic(true)
    {
    }

    Camera::Camera(float aspectRatio, float near, float far)
        : m_AspectRatio(aspectRatio)
        , m_Scale(1.0f)
        , m_FrustumDirty(true)
        , m_ProjectionDirty(true)
        , m_Fov(60)
        , m_Near(near)
        , m_Far(far)
        , m_Orthographic(true)
    {
    }

    void Camera::UpdateProjectionMatrix()
    {
        if(m_Orthographic)
            m_ProjMatrix = Mat4::Orthographic(-m_AspectRatio * m_Scale, m_AspectRatio * m_Scale, -m_Scale, m_Scale, m_Near, m_Far);
        else
            m_ProjMatrix = Mat4::Perspective(m_Near, m_Far, m_AspectRatio, m_Fov);
    }

    Maths::Frustum& Camera::GetFrustum(const Mat4& viewMatrix)
    {
        if(m_ProjectionDirty)
            UpdateProjectionMatrix();

        {
            m_Frustum.Define(m_ProjMatrix, viewMatrix);
            m_FrustumDirty = false;
        }

        return m_Frustum;
    }

    const Mat4& Camera::GetProjectionMatrix()
    {
        if(m_ProjectionDirty)
        {
            UpdateProjectionMatrix();
            m_ProjectionDirty = false;
        }
        return m_ProjMatrix;
    }

    Maths::Ray Camera::GetScreenRay(float x, float y, const Mat4& viewMatrix, bool flipY) const
    {
        Maths::Ray ret;
        Mat4 viewProjInverse = (m_ProjMatrix * viewMatrix).Inverse();

        x = 2.0f * x - 1.0f;
        y = 2.0f * y - 1.0f;

        if(flipY)
            y *= -1.0f;

        Vec4 n = viewProjInverse * Vec4(x, y, 0.0f, 1.0f);
        n /= n.w;

        Vec4 f = viewProjInverse * Vec4(x, y, 1.0f, 1.0f);
        f /= f.w;

        ret.Origin    = Vec3(n);
        ret.Direction = (Vec3(f) - ret.Origin).Normalised();

        return ret;
    }

}
