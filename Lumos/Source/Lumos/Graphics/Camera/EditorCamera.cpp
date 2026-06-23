#include "Precompiled.h"
#include "EditorCamera.h"
#include "Graphics/Camera/Camera.h"
#include "Core/Application.h"
#include "Core/OS/Input.h"
#include "Core/OS/Window.h"
#include "Maths/MathsUtilities.h"
#include "Maths/Transform.h"

namespace Lumos
{
    EditorCameraController::EditorCameraController()
    {
        m_FocalPoint            = Vec3();
        m_Velocity              = Vec3(0.0f);
        m_RotateVelocity        = Vec2(0.0f);
        m_PreviousCurserPos     = Vec2(0.0f);
        m_MouseSensitivity      = 0.00001f;
        m_ZoomDampeningFactor   = 0.00001f;
        m_DampeningFactor       = 0.00001f;
        m_RotateDampeningFactor = 0.0000001f;
        m_CameraMode            = EditorCameraMode::FLYCAM;
    }

    EditorCameraController::~EditorCameraController()
    {
    }

    void EditorCameraController::BeginCapture(float xpos, float ypos)
    {
        Application::Get().GetWindow()->HideMouse(true);
        Input::Get().SetMouseMode(MouseMode::Captured);
        m_StoredCursorPos   = Vec2(xpos, ypos);
        m_PreviousCurserPos = m_StoredCursorPos;
    }

    void EditorCameraController::EndCapture()
    {
        Application::Get().GetWindow()->HideMouse(false);
        Application::Get().GetWindow()->SetMousePosition(m_StoredCursorPos);
        Input::Get().SetMouseMode(MouseMode::Visible);
    }

    void EditorCameraController::UpdateCameraView(Maths::Transform& transform, float dt)
    {
        const float yawSign = transform.GetUpDirection().y < 0 ? -1.0f : 1.0f;

        // Extra step to handle the problem when the camera direction is the same as the up vector
        const float cosAngle = Maths::Dot(transform.GetForwardDirection(), transform.GetUpDirection());
        if(cosAngle * yawSign > 0.99f)
            m_PitchDelta = 0.f;

        // damping for smooth camera
        m_YawDelta *= pow(m_DampeningFactor, dt);
        m_PitchDelta *= pow(m_DampeningFactor, dt);
        m_PositionDelta *= pow(m_DampeningFactor, dt);
    }

    void EditorCameraController::HandleMouse(Maths::Transform& transform, float dt, float xpos, float ypos)
    {
        LUMOS_PROFILE_FUNCTION();

        m_Distance = Maths::Distance(transform.GetLocalPosition(), m_FocalPoint);

        Input& input = Input::Get();
        const bool altHeld   = input.GetKeyHeld(InputCode::Key::LeftAlt) || input.GetKeyHeld(InputCode::Key::RightAlt);
        const bool shiftHeld = input.GetKeyHeld(InputCode::Key::LeftShift);

        if(m_CameraMode == EditorCameraMode::TWODIM)
        {
            // 2D: RMB or MMB pans
            const bool panClicked = input.GetMouseClicked(InputCode::MouseKey::ButtonRight)
                                 || input.GetMouseClicked(InputCode::MouseKey::ButtonMiddle);
            const bool panHeld    = input.GetMouseHeld(InputCode::MouseKey::ButtonRight)
                                 || input.GetMouseHeld(InputCode::MouseKey::ButtonMiddle);

            if(panClicked && m_DragMode == DragMode::None)
            {
                m_DragMode = DragMode::Pan;
                BeginCapture(xpos, ypos);
            }

            if(m_DragMode == DragMode::Pan && panHeld)
            {
                m_MouseSensitivity = 0.05f;
                float speedScale   = shiftHeld ? 2.0f : 1.0f;
                Vec3 position      = transform.GetLocalPosition();
                position.x -= (xpos - m_PreviousCurserPos.x) * m_MouseSensitivity * 0.5f * speedScale;
                position.y += (ypos - m_PreviousCurserPos.y) * m_MouseSensitivity * 0.5f * speedScale;
                transform.SetLocalPosition(position);
                m_PreviousCurserPos = Vec2(xpos, ypos);
            }
            else if(m_DragMode != DragMode::None && !panHeld)
            {
                EndCapture();
                m_DragMode = DragMode::None;
            }

            UpdateScroll(transform, input.GetScrollOffset(), dt);
            return;
        }

        // -------- 3D modes (FLYCAM / ARCBALL) --------
        const bool rmbClicked = input.GetMouseClicked(InputCode::MouseKey::ButtonRight);
        const bool rmbHeld    = input.GetMouseHeld(InputCode::MouseKey::ButtonRight);
        const bool mmbClicked = input.GetMouseClicked(InputCode::MouseKey::ButtonMiddle);
        const bool mmbHeld    = input.GetMouseHeld(InputCode::MouseKey::ButtonMiddle);
        const bool lmbClicked = input.GetMouseClicked(InputCode::MouseKey::ButtonLeft);
        const bool lmbHeld    = input.GetMouseHeld(InputCode::MouseKey::ButtonLeft);

        // Choose drag mode on click
        if(m_DragMode == DragMode::None)
        {
            if(rmbClicked)
            {
                m_DragMode = DragMode::FreeLook;
                BeginCapture(xpos, ypos);
            }
            else if(mmbClicked)
            {
                m_DragMode = altHeld ? DragMode::Orbit : DragMode::Pan;
                BeginCapture(xpos, ypos);
            }
            else if(altHeld && lmbClicked)
            {
                m_DragMode = DragMode::Orbit;
                BeginCapture(xpos, ypos);
            }
        }

        // Process active drag
        if(m_DragMode != DragMode::None)
        {
            const bool stillHeld = (m_DragMode == DragMode::FreeLook && rmbHeld)
                                || (m_DragMode == DragMode::Pan && (mmbHeld || rmbHeld))
                                || (m_DragMode == DragMode::Orbit && (mmbHeld || lmbHeld));

            if(stillHeld)
            {
                Vec2 delta = Vec2(xpos - m_PreviousCurserPos.x, ypos - m_PreviousCurserPos.y);

                if(m_DragMode == DragMode::FreeLook)
                {
                    m_MouseSensitivity = 0.0002f;
                    m_RotateVelocity   = delta * m_MouseSensitivity * 10.0f;
                }
                else if(m_DragMode == DragMode::Pan)
                {
                    float speedScale = shiftHeld ? 3.0f : 1.0f;
                    MousePan(transform, delta * speedScale);
                }
                else if(m_DragMode == DragMode::Orbit)
                {
                    MouseOrbit(transform, delta * 0.003f);
                }

                m_PreviousCurserPos = Vec2(xpos, ypos);
            }
            else
            {
                EndCapture();
                m_DragMode = DragMode::None;
            }
        }

        const bool isOrbiting = (m_DragMode == DragMode::Orbit) || (m_CameraMode == EditorCameraMode::ARCBALL);
        const bool isFreeLook = (m_DragMode == DragMode::FreeLook);

        // FreeLook (RMB drag): apply rotation directly from velocity, no delta accumulator.
        if(isFreeLook && Maths::Length(m_RotateVelocity) > Maths::M_EPSILON)
        {
            Quat rotation  = transform.GetLocalOrientation();
            float pitch    = m_InvertY ? m_RotateVelocity.y : -m_RotateVelocity.y;
            Quat rotationX = Quat::Rotation(pitch, Vec3(1.0f, 0.0f, 0.0f));
            Quat rotationY = Quat::Rotation(-m_RotateVelocity.x, Vec3(0.0f, 1.0f, 0.0f));
            rotation = rotationY * rotation * rotationX;
            transform.SetLocalOrientation(rotation);
        }
        // Orbit / ARCBALL: use delta accumulators (allow inertia trail via UpdateCameraView damping).
        else if(isOrbiting && (Maths::Length(m_RotateVelocity) > Maths::M_EPSILON
                               || Maths::Abs(m_PitchDelta) > Maths::M_EPSILON
                               || Maths::Abs(m_YawDelta) > Maths::M_EPSILON))
        {
            MouseRotate(transform, m_RotateVelocity);

            Quat rotation  = transform.GetLocalOrientation();
            Quat rotationX = Quat::Rotation(-m_PitchDelta, Vec3(1.0f, 0.0f, 0.0f));
            Quat rotationY = Quat::Rotation(-m_YawDelta, Vec3(0.0f, 1.0f, 0.0f));
            rotation = rotationY * rotation * rotationX;
            transform.SetLocalOrientation(rotation);
        }

        // Damp velocity / clear stale state to prevent auto-rotation.
        m_RotateVelocity = m_RotateVelocity * pow(m_RotateDampeningFactor, dt);
        if(!isFreeLook)
            m_RotateVelocity = Vec2(0.0f);
        if(!isOrbiting)
        {
            m_PitchDelta = 0.0f;
            m_YawDelta   = 0.0f;
        }

        // Orbit / arcball position update — keep camera at focal distance
        if(isOrbiting)
        {
            MouseZoom(transform, input.GetScrollOffset());
            UpdateCameraView(transform, dt);
            transform.SetLocalPosition(CalculatePosition(transform));
        }
        else
        {
            UpdateScroll(transform, input.GetScrollOffset(), dt);
        }
    }

    void EditorCameraController::HandleKeyboard(Maths::Transform& transform, float dt)
    {
        if(m_CameraMode == EditorCameraMode::TWODIM)
        {
            Vec3 up = Vec3(0, 1, 0), right = Vec3(1, 0, 0);

            float speed = dt * m_CameraSpeed;

            if(Input::Get().GetKeyHeld(Lumos::InputCode::Key::A))
                m_Velocity -= right * speed;
            if(Input::Get().GetKeyHeld(Lumos::InputCode::Key::D))
                m_Velocity += right * speed;
            if(Input::Get().GetKeyHeld(Lumos::InputCode::Key::W))
                m_Velocity += up * speed;
            if(Input::Get().GetKeyHeld(Lumos::InputCode::Key::S))
                m_Velocity -= up * speed;

            if(Maths::Length(m_Velocity) > Maths::M_EPSILON)
            {
                Vec3 position = transform.GetLocalPosition();
                position += m_Velocity * dt;
                m_Velocity = m_Velocity * pow(m_DampeningFactor, dt);

                transform.SetLocalPosition(position);
            }
            return;
        }

        float multiplier = 1.0f;

        if(Input::Get().GetKeyHeld(InputCode::Key::LeftShift))
            multiplier = 10.0f;
        else if(Input::Get().GetKeyHeld(InputCode::Key::LeftAlt))
            multiplier = 0.5f;

        float speed = multiplier * dt * m_CameraSpeed;

        // WASD/QE while RMB held — fly mode (Unreal-style)
        if(Input::Get().GetMouseHeld(InputCode::MouseKey::ButtonRight))
        {
            if(Input::Get().GetKeyHeld(InputCode::Key::W))
                m_Velocity += transform.GetForwardDirection() * speed;
            if(Input::Get().GetKeyHeld(InputCode::Key::S))
                m_Velocity -= transform.GetForwardDirection() * speed;
            if(Input::Get().GetKeyHeld(InputCode::Key::A))
                m_Velocity -= transform.GetRightDirection() * speed;
            if(Input::Get().GetKeyHeld(InputCode::Key::D))
                m_Velocity += transform.GetRightDirection() * speed;
            if(Input::Get().GetKeyHeld(InputCode::Key::Q))
                m_Velocity -= transform.GetUpDirection() * speed;
            if(Input::Get().GetKeyHeld(InputCode::Key::E))
                m_Velocity += transform.GetUpDirection() * speed;
        }

        if(Input::Get().IsControllerPresent(0))
        {
            {
                float hAxis = Input::Get().GetControllerAxis(0, 0);
                float vAxis = Input::Get().GetControllerAxis(0, 1);

                if(Maths::Abs(vAxis) > 0.2f)
                    m_Velocity -= vAxis * transform.GetForwardDirection() * speed;
                if(Maths::Abs(hAxis) > 0.2f)
                    m_Velocity += hAxis * transform.GetRightDirection() * speed;
            }

            {
                float sensitivity = 0.2f;

                float hAxis = Input::Get().GetControllerAxis(0, 2);
                float vAxis = Input::Get().GetControllerAxis(0, 5);
                if(Maths::Abs(vAxis) < 0.2f)
                    vAxis = 0.0f;
                if(Maths::Abs(hAxis) < 0.2f)
                    hAxis = 0.0f;

                Vec2 delta = Vec2(hAxis * hAxis, vAxis * vAxis);
                delta *= Vec2(Maths::Sign(hAxis), Maths::Sign(vAxis));
                delta *= sensitivity;
                Quat rotationQ = transform.GetLocalOrientation();
                Quat rotationX = Quat::Rotation(-delta.y, Vec3(1.0f, 0.0f, 0.0f));
                Quat rotationY = Quat::Rotation(-delta.x, Vec3(0.0f, 1.0f, 0.0f));

                rotationQ = rotationY * rotationQ;
                rotationQ = rotationQ * rotationX;

                transform.SetLocalOrientation(rotationQ);
            }
        }

        if(Maths::Length(m_Velocity) > Maths::M_EPSILON)
        {
            Vec3 position = transform.GetLocalPosition();
            position += m_Velocity * dt;
            transform.SetLocalPosition(position);
            m_Velocity = m_Velocity * pow(m_DampeningFactor, dt);
        }
    }

    std::pair<float, float> EditorCameraController::PanSpeed() const
    {
        const float x       = Maths::Min(float(1920) / 1000.0f, 2.4f); // max = 2.4f
        const float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

        const float y       = Maths::Min(float(1080) / 1000.0f, 2.4f); // max = 2.4f
        const float yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

        return { xFactor, yFactor };
    }

    float EditorCameraController::RotationSpeed() const
    {
        return 0.3f;
    }

    float EditorCameraController::ZoomSpeed() const
    {
        float distance = m_Distance * 0.2f;
        distance       = Maths::Max(distance, 0.0f);
        float speed    = distance * distance;
        speed          = Maths::Min(speed, 50.0f);
        return speed * m_CameraSpeed / 10000.0f;
    }

    void EditorCameraController::MousePan(Maths::Transform& transform, const Vec2& delta)
    {
        float speedFactor = Maths::Max(m_CameraSpeed, 0.1f) / 20.0f; // 20 = default speed
        float panScale    = Maths::Max(m_Distance, 1.0f) * 0.001f * speedFactor;

        Vec3 right  = transform.GetRightDirection();
        Vec3 up     = transform.GetUpDirection();
        Vec3 offset = -right * delta.x * panScale + up * delta.y * panScale;

        m_FocalPoint += offset;
        if(m_CameraMode != EditorCameraMode::ARCBALL)
            transform.SetLocalPosition(transform.GetLocalPosition() + offset);
    }

    void EditorCameraController::MouseOrbit(Maths::Transform& transform, const Vec2& delta)
    {
        const float yawSign = transform.GetUpDirection().y < 0.0f ? -1.0f : 1.0f;
        float pitchInput    = m_InvertY ? -delta.y : delta.y;
        m_YawDelta   += yawSign * delta.x * RotationSpeed();
        m_PitchDelta += pitchInput * RotationSpeed();
    }

    void EditorCameraController::MouseRotate(Maths::Transform& transform, const Vec2& delta)
    {
        const float yawSign = transform.GetUpDirection().y < 0.0f ? -1.0f : 1.0f;
        float pitchInput    = m_InvertY ? -delta.y : delta.y;
        m_YawDelta   += yawSign * delta.x * RotationSpeed();
        m_PitchDelta += pitchInput * RotationSpeed();
    }

    void EditorCameraController::MouseZoom(Maths::Transform& transform, float delta)
    {
        if(delta == 0.0f)
            return;

        m_Distance -= delta * ZoomSpeed();
        const Vec3 forwardDir = transform.GetForwardDirection();
        if(m_Distance < 1.0f)
        {
            m_FocalPoint += forwardDir * m_Distance;
            m_Distance = 1.0f;
        }
        m_PositionDelta += delta * ZoomSpeed() * forwardDir;
    }

    void EditorCameraController::UpdateScroll(Maths::Transform& transform, float offset, float dt)
    {
        Input& input    = Input::Get();
        float offsetX   = input.GetScrollOffsetX();
        bool shiftHeld  = input.GetKeyHeld(InputCode::Key::LeftShift);
        bool altHeld    = input.GetKeyHeld(InputCode::Key::LeftAlt) || input.GetKeyHeld(InputCode::Key::RightAlt);
        bool ctrlHeld   = input.GetKeyHeld(InputCode::Key::LeftControl) || input.GetKeyHeld(InputCode::Key::LeftSuper);
        bool rmbHeld    = input.GetMouseHeld(InputCode::MouseKey::ButtonRight);

        if(m_CameraMode == EditorCameraMode::TWODIM)
        {
            if(!m_Camera)
                return;

            float multiplier = m_CameraSpeed / 10.0f;
            if(shiftHeld)
                multiplier = m_CameraSpeed / 2.0f;

            if(offset != 0.0f)
                m_ZoomVelocity += dt * offset * multiplier;

            if(!Maths::Equals(m_ZoomVelocity, 0.0f))
            {
                float scale = m_Camera->GetScale();
                scale -= m_ZoomVelocity;

                if(scale < 0.15f)
                {
                    scale          = 0.15f;
                    m_ZoomVelocity = 0.0f;
                }
                else
                {
                    m_ZoomVelocity = m_ZoomVelocity * pow(m_ZoomDampeningFactor, dt);
                }

                m_Camera->SetScale(scale);
            }
            return;
        }

        // RMB+scroll or Cmd/Ctrl+scroll = adjust camera speed (affects fly + pan speeds).
        // Cmd/Ctrl path is the Mac-friendly alternative since RMB-drag often takes priority for context menus.
        if((rmbHeld || ctrlHeld) && offset != 0.0f)
        {
            m_CameraSpeed += dt * offset * (m_CameraSpeed * 0.5f + 1.0f);
            m_CameraSpeed = Maths::Clamp(m_CameraSpeed, 0.1f, 1000.0f);
            return;
        }

        // Orbit only on explicit Alt+scroll. Trackpad auto-detect was unreliable
        // (false-positives from smooth-scroll mice; falsely flickering during pure-Y gestures).
        if(altHeld && (offset != 0.0f || offsetX != 0.0f))
        {
            const float yawSign = transform.GetUpDirection().y < 0.0f ? -1.0f : 1.0f;
            float yawAmt        = yawSign * offsetX * RotationSpeed() * 0.05f;
            float pitchInput    = m_InvertY ? -offset : offset;
            float pitchAmt      = pitchInput * RotationSpeed() * 0.05f;

            Quat rotation  = transform.GetLocalOrientation();
            Quat rotationX = Quat::Rotation(-pitchAmt, Vec3(1.0f, 0.0f, 0.0f));
            Quat rotationY = Quat::Rotation(-yawAmt, Vec3(0.0f, 1.0f, 0.0f));
            rotation = rotationY * rotation * rotationX;
            transform.SetLocalOrientation(rotation);

            // Reposition on orbit sphere around focal point
            m_Distance      = Maths::Max(m_Distance, 1.0f);
            Vec3 forwardDir = transform.GetForwardDirection();
            transform.SetLocalPosition(m_FocalPoint - forwardDir * m_Distance);
            return;
        }

        // Shift+scroll = pan vertically (move focal point + position)
        if(shiftHeld && offset != 0.0f)
        {
            float panAmount = offset * Maths::Max(m_Distance, 1.0f) * 0.05f;
            Vec3 up         = transform.GetUpDirection();
            Vec3 delta      = up * panAmount;
            m_FocalPoint += delta;
            transform.SetLocalPosition(transform.GetLocalPosition() + delta);
            return;
        }


        // Default: dolly forward/backward
        if(offset != 0.0f)
            m_ZoomVelocity += dt * offset * m_CameraSpeed * 0.01f;

        if(!Maths::Equals(m_ZoomVelocity, 0.0f))
        {
            Vec3 pos = transform.GetLocalPosition();
            Vec3 fwd = transform.GetForwardDirection();
            pos += fwd * m_ZoomVelocity;
            // Keep focal point updated so subsequent pan/orbit feels coherent
            m_FocalPoint += fwd * m_ZoomVelocity;
            m_ZoomVelocity = m_ZoomVelocity * pow(m_ZoomDampeningFactor, dt);

            transform.SetLocalPosition(pos);
        }
    }

    void EditorCameraController::HandleGesturePinch(Maths::Transform& transform, float scale, float velocity, float dt)
    {
        if(m_CameraMode == EditorCameraMode::TWODIM)
        {
            if(m_Camera)
            {
                float newScale = m_Camera->GetScale() / scale;
                m_Camera->SetScale(Maths::Clamp(newScale, 0.1f, 100.0f));
            }
            return;
        }

        float zoomAmount = (scale - 1.0f) * Maths::Max(m_Distance, 1.0f) * 2.0f;
        Vec3 fwd         = transform.GetForwardDirection();
        Vec3 pos         = transform.GetLocalPosition() + fwd * zoomAmount;
        m_FocalPoint    += fwd * zoomAmount;
        transform.SetLocalPosition(pos);
    }

    void EditorCameraController::HandleGesturePan(Maths::Transform& transform, const Vec2& delta, const Vec2& velocity, uint32_t touchCount)
    {
        Vec2 scaledDelta = delta * 0.002f;

        if(m_CameraMode == EditorCameraMode::TWODIM)
        {
            Vec3 right = transform.GetRightDirection();
            Vec3 up    = transform.GetUpDirection();
            Vec3 d     = right * scaledDelta.x * 5.0f + up * -scaledDelta.y * 5.0f;
            transform.SetLocalPosition(transform.GetLocalPosition() + d);
            return;
        }

        // 3-finger drag = pan focal point (and follow with camera)
        if(touchCount >= 3)
        {
            float panScale = Maths::Max(m_Distance, 1.0f);
            Vec3 right     = transform.GetRightDirection();
            Vec3 up        = transform.GetUpDirection();
            Vec3 d         = -right * scaledDelta.x * panScale + up * scaledDelta.y * panScale;
            m_FocalPoint  += d;
            transform.SetLocalPosition(transform.GetLocalPosition() + d);
            return;
        }

        // 2-finger drag = rotate (free-look)
        const float yawSign = transform.GetUpDirection().y < 0.0f ? -1.0f : 1.0f;
        float yawDelta      = yawSign * scaledDelta.x * RotationSpeed();
        float pitchInput    = m_InvertY ? -scaledDelta.y : scaledDelta.y;
        float pitchDelta    = pitchInput * RotationSpeed();

        Quat rotation  = transform.GetLocalOrientation();
        Quat rotationX = Quat::Rotation(-pitchDelta, Vec3(1.0f, 0.0f, 0.0f));
        Quat rotationY = Quat::Rotation(-yawDelta, Vec3(0.0f, 1.0f, 0.0f));

        rotation = rotationY * rotation;
        rotation = rotation * rotationX;
        transform.SetLocalOrientation(rotation);
    }

    void EditorCameraController::StopMovement()
    {
        m_ZoomVelocity   = 0.0f;
        m_Velocity       = Vec3(0.0f);
        m_RotateVelocity = Vec2(0.0f);
        m_PitchDelta     = 0.0f;
        m_YawDelta       = 0.0f;
        m_PositionDelta  = Vec3(0.0f);

        if(m_DragMode != DragMode::None)
        {
            EndCapture();
            m_DragMode = DragMode::None;
        }
    }

    Vec3 EditorCameraController::CalculatePosition(Maths::Transform& transform) const
    {
        return m_FocalPoint + transform.GetForwardDirection() * m_Distance + m_PositionDelta;
    }
}
