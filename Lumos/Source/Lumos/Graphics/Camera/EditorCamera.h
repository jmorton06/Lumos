#pragma once
#include "Graphics/Camera/CameraController.h"

namespace Lumos
{
    enum class EditorCameraMode
    {
        NONE,
        FLYCAM,
        ARCBALL,
        TWODIM
    };

    class LUMOS_EXPORT EditorCameraController : public CameraController
    {
    public:
        EditorCameraController();
        ~EditorCameraController();

        virtual void HandleMouse(Maths::Transform& transform, float dt, float xpos, float ypos) override;
        virtual void HandleKeyboard(Maths::Transform& transform, float dt) override;

        void MousePan(Maths::Transform& transform, const Vec2& delta);
        void MouseOrbit(Maths::Transform& transform, const Vec2& delta);
        void MouseRotate(Maths::Transform& transform, const Vec2& delta);
        void MouseZoom(Maths::Transform& transform, float delta);
        void UpdateCameraView(Maths::Transform& transform, float dt);

        Vec3 CalculatePosition(Maths::Transform& transform) const;
        std::pair<float, float> PanSpeed() const;
        float RotationSpeed() const;
        float ZoomSpeed() const;

        void UpdateScroll(Maths::Transform& transform, float offset, float dt) override;

        void HandleGesturePinch(Maths::Transform& transform, float scale, float velocity, float dt);
        void HandleGesturePan(Maths::Transform& transform, const Vec2& delta, const Vec2& velocity, uint32_t touchCount = 2);

        void StopMovement();
        void SetSpeed(float speed) { m_CameraSpeed = speed; }
        float GetSpeed() const { return m_CameraSpeed; }

        void SetCurrentMode(EditorCameraMode mode) { m_CameraMode = mode; }
        EditorCameraMode GetCurrentMode() const { return m_CameraMode; }

        void SetFocalPoint(const Vec3& p) { m_FocalPoint = p; }
        const Vec3& GetFocalPoint() const { return m_FocalPoint; }

        void SetInvertY(bool b) { m_InvertY = b; }
        bool GetInvertY() const { return m_InvertY; }

    private:
        // Begin a captured drag (hide cursor + remember restore position)
        void BeginCapture(float xpos, float ypos);
        // End captured drag (restore cursor)
        void EndCapture();

        EditorCameraMode m_CameraMode = EditorCameraMode::ARCBALL;
        Vec2 m_StoredCursorPos;
        float m_CameraSpeed = 20.0f;

        float m_PitchDelta { 0.0f }, m_YawDelta { 0.0f };
        Vec3 m_PositionDelta {};

        // Drag state — which button started the current capture (0=none, 1=RMB, 2=MMB, 3=LMB-orbit)
        enum class DragMode { None, FreeLook, Pan, Orbit };
        DragMode m_DragMode = DragMode::None;

        bool m_InvertY = false;
    };
}
