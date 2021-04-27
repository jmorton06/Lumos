#pragma once

#include "CameraController.h"

#include "Maths/Maths.h"
#include "Maths/Ray.h"

#include <cereal/cereal.hpp>

namespace Lumos
{
    enum RenderPath
    {
        Forward,
        Deferred,
        Batch2D
    };

    enum RenderTarget
    {
        Texture,
        Display0
    };

    class LUMOS_EXPORT Camera
    {
    public:
        Camera() = default;
        Camera(float FOV, float Near, float Far, float aspect);
        Camera(float pitch, float yaw, const Maths::Vector3& position, float FOV, float Near, float Far, float aspect);
        Camera(float aspectRatio, float scale);

        ~Camera() = default;

        void SetMouseSensitivity(float value)
        {
            m_MouseSensitivity = value;
        }

        void SetIsOrthographic(bool ortho)
        {
            m_FrustumDirty = true;
            m_ProjectionDirty = true;
            m_Orthographic = ortho;
        }

        bool IsOrthographic() const
        {
            return m_Orthographic;
        }

        float GetAspectRatio() const
        {
            return m_AspectRatio;
        }

        void SetAspectRatio(float y)
        {
            m_AspectRatio = y;
            m_ProjectionDirty = true;
            m_FrustumDirty = true;
        };

        const Maths::Matrix4& GetProjectionMatrix();

        float GetFar() const
        {
            return m_Far;
        }
        float GetNear() const
        {
            return m_Near;
        }

        void SetFar(float pFar)
        {
            m_Far = pFar;
            m_ProjectionDirty = true;
            m_FrustumDirty = true;
        }

        void SetNear(float pNear)
        {
            m_Near = pNear;
            m_ProjectionDirty = true;
            m_FrustumDirty = true;
        }

        float GetFOV() const
        {
            return m_Fov;
        }

        float GetScale() const
        {
            return m_Scale;
        }

        void SetScale(float scale)
        {
            m_Scale = scale;
            m_ProjectionDirty = true;
            m_FrustumDirty = true;
        }

        void SetFOV(float fov)
        {
            m_Fov = fov;
            m_ProjectionDirty = true;
            m_FrustumDirty = true;
        }

        Maths::Frustum& GetFrustum(const Maths::Matrix4& viewMatrix);

        Maths::Ray GetScreenRay(float x, float y, const Maths::Matrix4& viewMatrix, bool invertY = false) const;

        template <typename Archive>
        void save(Archive& archive) const
        {
            archive(cereal::make_nvp("Scale", m_Scale), cereal::make_nvp("Aspect", m_AspectRatio), cereal::make_nvp("FOV", m_Fov), cereal::make_nvp("Near", m_Near), cereal::make_nvp("Far", m_Far));
        }

        template <typename Archive>
        void load(Archive& archive)
        {
            archive(cereal::make_nvp("Scale", m_Scale), cereal::make_nvp("Aspect", m_AspectRatio), cereal::make_nvp("FOV", m_Fov), cereal::make_nvp("Near", m_Near), cereal::make_nvp("Far", m_Far));

            m_FrustumDirty = true;
            m_ProjectionDirty = true;
        }

        float GetShadowBoundingRadius() const
        {
            return m_ShadowBoundingRadius;
        }

    protected:
        void UpdateProjectionMatrix();

        float m_ShadowBoundingRadius = 25.0f;

        float m_AspectRatio = 0.0f;
        float m_Scale = 1.0f;
        float m_Zoom = 1.0f;

        Maths::Vector2 m_ProjectionOffset = Maths::Vector2(0.0f, 0.0f);

        Maths::Matrix4 m_ProjMatrix;

        Maths::Frustum m_Frustum;
        bool m_FrustumDirty = true;
        bool m_ProjectionDirty = false;
        bool customProjection_ = false;

        float m_Fov = 0.0f, m_Near = 0.0f, m_Far = 0.0f;
        float m_MouseSensitivity = 0.1f;

        bool m_Orthographic = false;
        RenderPath m_RenderPath = RenderPath::Deferred;
        bool m_CastShadow = true;
        RenderTarget m_Target = RenderTarget::Display0;
    };

}
