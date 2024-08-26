#pragma once
#include "Maths/Vector2.h"
#include "Maths/Vector3.h"

namespace Lumos
{
    namespace Maths
    {
        class Transform;
    }
    class Camera;

    class LUMOS_EXPORT CameraController
    {
    public:
        CameraController()          = default;
        virtual ~CameraController() = default;

        virtual void HandleMouse(Maths::Transform& transform, float dt, float xpos, float ypos) {};
        virtual void HandleKeyboard(Maths::Transform& transform, float dt) {};
        virtual void UpdateScroll(Maths::Transform& transform, float offset, float dt) {};

        virtual void OnImGui() {};

        void SetMouseSensitivity(float value) { m_MouseSensitivity = value; }

        const Vec3& GetVelocity() const { return m_Velocity; }

        void SetCamera(Camera* camera) { m_Camera = camera; }

    protected:
        Vec3 m_Velocity;
        Vec2 m_RotateVelocity;
        Vec2 m_ArcRotateVelocity;
        Vec3 m_FocalPoint;

        float m_ZoomVelocity = 0.0f;
        float m_CameraSpeed  = 0.0f;
        float m_Distance     = 0.0f;
        float m_Zoom         = 1.0f;

        Vec2 m_ProjectionOffset  = Vec2(0.0f, 0.0f);
        Vec2 m_PreviousCurserPos = Vec2(0.0f, 0.0f);
        float m_MouseSensitivity = 0.1f;

        float m_ZoomDampeningFactor   = 0.00001f;
        float m_DampeningFactor       = 0.00001f;
        float m_RotateDampeningFactor = 0.001f;

        Camera* m_Camera = nullptr;
    };

}
