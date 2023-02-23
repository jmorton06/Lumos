#include "Precompiled.h"
#include "EditorCamera.h"
#include "Graphics/Camera/Camera.h"
#include "Core/Application.h"
#include "Core/OS/Input.h"
#include "Core/OS/Window.h"

namespace Lumos
{
    EditorCameraController::EditorCameraController()
    {
        m_FocalPoint            = glm::vec3();
        m_Velocity              = glm::vec3(0.0f);
        m_RotateVelocity        = glm::vec2(0.0f);
        m_PreviousCurserPos     = glm::vec3(0.0f);
        m_MouseSensitivity      = 0.00001f;
        m_ZoomDampeningFactor   = 0.00001f;
        m_DampeningFactor       = 0.00001f;
        m_RotateDampeningFactor = 0.0000001f;
        m_CameraMode = EditorCameraMode::FLYCAM;
    }

    EditorCameraController::~EditorCameraController()
    {
    }

    void EditorCameraController::UpdateCameraView(Maths::Transform& transform, float dt)
    {
        const float yawSign = transform.GetUpDirection().y < 0 ? -1.0f : 1.0f;

        // Extra step to handle the problem when the camera direction is the same as the up vector
        const float cosAngle = glm::dot(transform.GetForwardDirection(), transform.GetUpDirection());
        if (cosAngle * yawSign > 0.99f)
            m_PitchDelta = 0.f;

        //damping for smooth camera
        m_YawDelta *= pow(m_DampeningFactor, dt);
        m_PitchDelta *= pow(m_DampeningFactor, dt);
        m_PositionDelta *= pow(m_DampeningFactor, dt);
    }
    void EditorCameraController::HandleMouse(Maths::Transform& transform, float dt, float xpos, float ypos)
{
        LUMOS_PROFILE_FUNCTION();
        
        m_Distance = glm::distance(transform.GetLocalPosition(), m_FocalPoint);
        
        
        if(m_CameraMode == EditorCameraMode::TWODIM)
        {
            if(Input::Get().GetMouseHeld(InputCode::MouseKey::ButtonRight))
            {
                m_MouseSensitivity = 0.0005f;
                glm::vec3 position = transform.GetLocalPosition();
                position.x -= (xpos - m_PreviousCurserPos.x) /** camera->GetScale() */ * m_MouseSensitivity * 0.5f;
                position.y += (ypos - m_PreviousCurserPos.y) /** camera->GetScale() */ * m_MouseSensitivity * 0.5f;
                transform.SetLocalPosition(position);
            }
        }
        else
        {
            static bool mouseHeld = false;
            if(Input::Get().GetMouseClicked(InputCode::MouseKey::ButtonRight))
            {
                mouseHeld = true;
                Application::Get().GetWindow()->HideMouse(true);
                Input::Get().SetMouseMode(MouseMode::Captured);
                m_StoredCursorPos   = glm::vec2(xpos, ypos);
                m_PreviousCurserPos = m_StoredCursorPos;
            }
            
            if(Input::Get().GetMouseHeld(InputCode::MouseKey::ButtonRight))
            {
                m_MouseSensitivity = 0.0002f;
                m_RotateVelocity   = glm::vec2((xpos - m_PreviousCurserPos.x), (ypos - m_PreviousCurserPos.y)) * m_MouseSensitivity * 10.0f;
            }
            else
            {
                if(mouseHeld)
                {
                    mouseHeld = false;
                    Application::Get().GetWindow()->HideMouse(false);
                    Application::Get().GetWindow()->SetMousePosition(m_StoredCursorPos);
                    Input::Get().SetMouseMode(MouseMode::Visible);
                }
                else if(Input::Get().GetMouseHeld(InputCode::MouseKey::ButtonMiddle))
                    MousePan(transform, glm::vec2((xpos - m_PreviousCurserPos.x), (ypos - m_PreviousCurserPos.y)));

            }
            
            if(glm::length(m_RotateVelocity) > Maths::M_EPSILON || m_PitchDelta > Maths::M_EPSILON ||  m_YawDelta > Maths::M_EPSILON)
            {
                if(m_CameraMode == EditorCameraMode::ARCBALL)
                {
                    MouseRotate(transform, m_RotateVelocity);
                    m_PreviousCurserPos = glm::vec2(xpos, ypos);
                    
                    glm::quat rotation  = transform.GetLocalOrientation();
                    glm::quat rotationX = glm::angleAxis(-m_PitchDelta, glm::vec3(1.0f, 0.0f, 0.0f));
                    glm::quat rotationY = glm::angleAxis(-m_YawDelta, glm::vec3(0.0f, 1.0f, 0.0f));
                    
                    rotation = rotationY * rotation;
                    rotation = rotation * rotationX;
                    transform.SetLocalOrientation(rotation);
                    
                    //UpdateCameraView(transform, dt);
                }
                else if(m_CameraMode == EditorCameraMode::FLYCAM)
                {
                    glm::quat rotation  = transform.GetLocalOrientation();
                    glm::quat rotationX = glm::angleAxis(-m_RotateVelocity.y, glm::vec3(1.0f, 0.0f, 0.0f));
                    glm::quat rotationY = glm::angleAxis(-m_RotateVelocity.x, glm::vec3(0.0f, 1.0f, 0.0f));
                    
                    rotation = rotationY * rotation;
                    rotation = rotation * rotationX;
                    
                    m_PreviousCurserPos = glm::vec2(xpos, ypos);
                    transform.SetLocalOrientation(rotation);
                }
            }
        }
        
        m_RotateVelocity = m_RotateVelocity * pow(m_RotateDampeningFactor, dt);
        
        if(m_CameraMode == EditorCameraMode::ARCBALL)
        {
            MouseZoom(transform, Input::Get().GetScrollOffset());
            UpdateCameraView(transform, dt);
            if (m_CameraMode == EditorCameraMode::ARCBALL)
                transform.SetLocalPosition(CalculatePosition(transform));
            
            //fUpdateScroll(transform, Input::Get().GetScrollOffset(), dt);
        }
        else if(m_CameraMode == EditorCameraMode::FLYCAM)
        {
            UpdateScroll(transform, Input::Get().GetScrollOffset(), dt);
        }
    }

    void EditorCameraController::HandleKeyboard(Maths::Transform& transform, float dt)
    {
        if(m_CameraMode == EditorCameraMode::TWODIM)
        {
            glm::vec3 up = glm::vec3(0, 1, 0), right = glm::vec3(1, 0, 0);

            float speed = /*camera->GetScale() **/ dt * m_CameraSpeed;

            if(Input::Get().GetKeyHeld(Lumos::InputCode::Key::A))
            {
                m_Velocity -= right * speed;
            }

            if(Input::Get().GetKeyHeld(Lumos::InputCode::Key::D))
            {
                m_Velocity += right * speed;
            }

            if(Input::Get().GetKeyHeld(Lumos::InputCode::Key::W))
            {
                m_Velocity += up * speed;
            }

            if(Input::Get().GetKeyHeld(Lumos::InputCode::Key::S))
            {
                m_Velocity -= up * speed;
            }

            if(glm::length(m_Velocity) > Maths::M_EPSILON)
            {
                glm::vec3 position = transform.GetLocalPosition();
                position += m_Velocity * dt;
                m_Velocity = m_Velocity * pow(m_DampeningFactor, dt);

                transform.SetLocalPosition(position);
            }
        }
        else
        {

            float multiplier = 1.0f;

            if(Input::Get().GetKeyHeld(InputCode::Key::LeftShift))
            {
                multiplier = 10.0f;
            }
            else if(Input::Get().GetKeyHeld(InputCode::Key::LeftAlt))
            {
                multiplier = 0.5f;
            }

            float speed = multiplier * dt * m_CameraSpeed;

            if(Input::Get().GetMouseHeld(InputCode::MouseKey::ButtonRight))
            {
                if(Input::Get().GetKeyHeld(InputCode::Key::W))
                {
                    m_Velocity -= transform.GetForwardDirection() * speed;
                }

                if(Input::Get().GetKeyHeld(InputCode::Key::S))
                {
                    m_Velocity += transform.GetForwardDirection() * speed;
                }

                if(Input::Get().GetKeyHeld(InputCode::Key::A))
                {
                    m_Velocity -= transform.GetRightDirection() * speed;
                }

                if(Input::Get().GetKeyHeld(InputCode::Key::D))
                {
                    m_Velocity += transform.GetRightDirection() * speed;
                }

                if(Input::Get().GetKeyHeld(InputCode::Key::Q))
                {
                    m_Velocity -= transform.GetUpDirection() * speed;
                }

                if(Input::Get().GetKeyHeld(InputCode::Key::E))
                {
                    m_Velocity += transform.GetUpDirection() * speed;
                }
            }

            if(m_Velocity.length() > Maths::M_EPSILON)
            {
                glm::vec3 position = transform.GetLocalPosition();
                position += m_Velocity * dt;
                transform.SetLocalPosition(position);
                m_Velocity = m_Velocity * pow(m_DampeningFactor, dt);
            }
        }
    }

    std::pair<float, float> EditorCameraController::PanSpeed() const
    {
        const float x = glm::min(float(1920) / 1000.0f, 2.4f); // max = 2.4f
        const float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

        const float y = glm::min(float(1080) / 1000.0f, 2.4f); // max = 2.4f
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
        distance = glm::max(distance, 0.0f);
        float speed = distance * distance;
        speed = glm::min(speed, 50.0f); // max speed = 50
        return speed * m_CameraSpeed / 10000.0f;
    }
    void EditorCameraController::MousePan(Maths::Transform& transform,const glm::vec2& delta)
    {
        auto [xSpeed, ySpeed] = PanSpeed();
        m_FocalPoint -= transform.GetRightDirection() * delta.x * xSpeed * m_Distance;
        m_FocalPoint += transform.GetUpDirection() * delta.y * ySpeed * m_Distance;
    }

    void EditorCameraController::MouseRotate(Maths::Transform& transform,const glm::vec2& delta)
    {
        const float yawSign = transform.GetUpDirection().y < 0.0f ? -1.0f : 1.0f;
        m_YawDelta += yawSign * delta.x * RotationSpeed();
        m_PitchDelta += delta.y * RotationSpeed();
    }

    void EditorCameraController::MouseZoom(Maths::Transform& transform,float delta)
    {
        if(delta == 0.0f)
            return;
        
        m_Distance -= delta * ZoomSpeed();
        const glm::vec3 forwardDir = transform.GetForwardDirection();
        //transform.SetLocalPosition(m_FocalPoint - forwardDir * m_Distance);
        if (m_Distance < 1.0f)
        {
            m_FocalPoint += forwardDir * m_Distance;
            m_Distance = 1.0f;
        }
        m_PositionDelta += delta * ZoomSpeed() * forwardDir;
    }


    void EditorCameraController::UpdateScroll(Maths::Transform& transform, float offset, float dt)
    {
        if(m_CameraMode == EditorCameraMode::TWODIM)
        {
            float multiplier = m_CameraSpeed / 10.0f;
            if(Input::Get().GetKeyHeld(InputCode::Key::LeftShift))
            {
                multiplier = m_CameraSpeed / 2.0f;
            }

            if(offset != 0.0f)
            {
                m_ZoomVelocity += dt * offset * multiplier;
            }

            if(!Maths::Equals(m_ZoomVelocity, 0.0f))
            {
                float scale = 1.0f; // camera->GetScale();

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

                // camera->SetScale(scale);
            }
        }
        else
        {

            if(offset != 0.0f)
            {
                m_ZoomVelocity -= dt * offset * 10.0f;
            }

            if(!Maths::Equals(m_ZoomVelocity, 0.0f))
            {
                glm::vec3 pos = transform.GetLocalPosition();
                pos += transform.GetForwardDirection() * m_ZoomVelocity;
                m_ZoomVelocity = m_ZoomVelocity * pow(m_ZoomDampeningFactor, dt);

                transform.SetLocalPosition(pos);
            }
        }
    }

    void EditorCameraController::StopMovement()
    {
        m_ZoomVelocity   = 0.0f;
        m_Velocity       = glm::vec3(0.0f);
        m_RotateVelocity = glm::vec2(0.0f);
    }

    glm::vec3 EditorCameraController::CalculatePosition(Maths::Transform& transform) const
    {
        return m_FocalPoint + transform.GetForwardDirection() * m_Distance + m_PositionDelta;
    }
}
