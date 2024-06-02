#pragma once

#include "CameraController.h"
#include "Maths/Ray.h"
#include "Maths/Frustum.h"
#include "Scene/Serialisation/Serialisation.h"

namespace Lumos
{
    class LUMOS_EXPORT Camera
    {
    public:
        Camera();
        Camera(float FOV, float Near, float Far, float aspect);
        Camera(float pitch, float yaw, const glm::vec3& position, float FOV, float Near, float Far, float aspect);
        Camera(float aspectRatio, float scale);
        Camera(float aspectRatio, float near, float far);
        ~Camera() = default;

        void SetMouseSensitivity(float value)
        {
            m_MouseSensitivity = value;
        }

        void SetIsOrthographic(bool ortho)
        {
            m_FrustumDirty    = true;
            m_ProjectionDirty = true;
            m_Orthographic    = ortho;
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
            m_AspectRatio     = y;
            m_ProjectionDirty = true;
            m_FrustumDirty    = true;
        };

        const glm::mat4& GetProjectionMatrix();

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
            m_Far             = pFar;
            m_ProjectionDirty = true;
            m_FrustumDirty    = true;
        }

        void SetNear(float pNear)
        {
            m_Near            = pNear;
            m_ProjectionDirty = true;
            m_FrustumDirty    = true;
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
            m_Scale           = scale;
            m_ProjectionDirty = true;
            m_FrustumDirty    = true;
        }

        void SetFOV(float fov)
        {
            m_Fov             = fov;
            m_ProjectionDirty = true;
            m_FrustumDirty    = true;
        }

        Maths::Frustum& GetFrustum(const glm::mat4& viewMatrix);

        Maths::Ray GetScreenRay(float x, float y, const glm::mat4& viewMatrix, bool flipY) const;

        template <typename Archive>
        void save(Archive& archive) const
        {
            archive(cereal::make_nvp("Scale", m_Scale), cereal::make_nvp("Aspect", m_AspectRatio), cereal::make_nvp("FOV", m_Fov), cereal::make_nvp("Near", m_Near), cereal::make_nvp("Far", m_Far));

            archive(cereal::make_nvp("Aperture", m_Aperture), cereal::make_nvp("ShutterSpeed", m_ShutterSpeed), cereal::make_nvp("Sensitivity", m_Sensitivity));
        }

        template <typename Archive>
        void load(Archive& archive)
        {
            archive(cereal::make_nvp("Scale", m_Scale), cereal::make_nvp("Aspect", m_AspectRatio), cereal::make_nvp("FOV", m_Fov), cereal::make_nvp("Near", m_Near), cereal::make_nvp("Far", m_Far));

            if(Serialisation::CurrentSceneVersion > 11)
            {
                archive(cereal::make_nvp("Aperture", m_Aperture), cereal::make_nvp("ShutterSpeed", m_ShutterSpeed), cereal::make_nvp("Sensitivity", m_Sensitivity));
            }

            m_FrustumDirty    = true;
            m_ProjectionDirty = true;
        }

        float GetShadowBoundingRadius() const
        {
            return m_ShadowBoundingRadius;
        }

        float GetAperture() const { return m_Aperture; }
        void SetAperture(const float aperture) { m_Aperture = aperture; }

        float GetShutterSpeed() const { return m_ShutterSpeed; }
        void SetShutterSpeed(const float shutterSpeed) { m_ShutterSpeed = shutterSpeed; }

        float GetSensitivity() const { return m_Sensitivity; }
        void SetSensitivity(const float Sensitivity) { m_Sensitivity = Sensitivity; }

        // https://google.github.io/filament/Filament.html
        float GetEv100() const { return std::log2((m_Aperture * m_Aperture) / m_ShutterSpeed * 100.0f / m_Sensitivity); }
        float GetExposure() const { return 1.0f / (std::pow(2.0f, GetEv100()) * 1.2f); }

    protected:
        void UpdateProjectionMatrix();

        float m_ShadowBoundingRadius = 10.0f;
        float m_AspectRatio          = 1.0f;
        float m_Scale                = 1.0f;
        float m_Zoom                 = 1.0f;

        glm::vec2 m_ProjectionOffset = glm::vec2(0.0f, 0.0f);

        glm::mat4 m_ProjMatrix;

        Maths::Frustum m_Frustum;
        bool m_FrustumDirty     = true;
        bool m_ProjectionDirty  = false;
        bool m_CustomProjection = false;

        float m_Fov = 60.0f, m_Near = 0.001f, m_Far = 5000.0f;
        float m_MouseSensitivity = 0.1f;
        float m_Aperture         = 55.0f;
        float m_ShutterSpeed     = 1.0f / 60.0f;
        float m_Sensitivity      = 250.0f;

        bool m_Orthographic = false;
    };
}
