#pragma once
#include "Graphics/Camera/CameraController.h"

namespace Lumos
{
    enum class EditorCameraMode
    {
        NONE, FLYCAM, ARCBALL, TWODIM
    };

    class LUMOS_EXPORT EditorCameraController : public CameraController
    {
    public:
        EditorCameraController();
        ~EditorCameraController();

        virtual void HandleMouse(Maths::Transform& transform, float dt, float xpos, float ypos) override;
        virtual void HandleKeyboard(Maths::Transform& transform, float dt) override;

        void MousePan(Maths::Transform& transform,const glm::vec2& delta);
        void MouseRotate(Maths::Transform& transform,const glm::vec2& delta);
        void MouseZoom(Maths::Transform& transform,float delta);
        void UpdateCameraView(Maths::Transform& transform, float dt);

        glm::vec3 CalculatePosition(Maths::Transform& transform) const;
        std::pair<float, float> PanSpeed() const;
        float RotationSpeed() const;
        float ZoomSpeed() const;
        
        void UpdateScroll(Maths::Transform& transform, float offset, float dt) override;

        void StopMovement();
        void SetSpeed(float speed) { m_CameraSpeed = speed; }

        void SetCurrentMode(EditorCameraMode mode) { m_CameraMode = mode; }
        EditorCameraMode GetCurrentMode() const { return m_CameraMode; }
        
    private:
        EditorCameraMode m_CameraMode = EditorCameraMode::ARCBALL;
        glm::vec2 m_StoredCursorPos;
        float m_CameraSpeed = 20.0f;
        
        float m_PitchDelta{0.0f}, m_YawDelta{0.0f};
        glm::vec3 m_PositionDelta{};
        
    };
}
