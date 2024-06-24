#ifndef LUMOS_PLATFORM_MACOS
#include "Precompiled.h"
#endif
#include "FPSCamera.h"
#include "Core/Application.h"
#include "Core/OS/Input.h"
#include "Core/OS/Window.h"
#include "Camera.h"
#include "Maths/MathsUtilities.h"
#include "Maths/Transform.h"

namespace Lumos
{

    FPSCameraController::FPSCameraController()
    {
    }

    FPSCameraController::~FPSCameraController() = default;

    void FPSCameraController::HandleMouse(Maths::Transform& transform, float dt, float xpos, float ypos)
    {
        if(Application::Get().GetWindow()->GetWindowFocus())
        {
            {
                glm::vec2 windowCentre = glm::vec2();
                xpos -= windowCentre.x;
                ypos -= windowCentre.y;

                Application::Get().GetWindow()->SetMousePosition(windowCentre);

                glm::vec3 euler = glm::eulerAngles(transform.GetLocalOrientation());
                float pitch     = euler.x;
                float yaw       = euler.y;

                pitch -= (ypos)*m_MouseSensitivity;
                yaw -= (xpos)*m_MouseSensitivity;

                transform.SetLocalOrientation(glm::quat(glm::vec3(pitch, yaw, euler.z)));
            }

            m_PreviousCurserPos = glm::vec2(xpos, ypos);

            UpdateScroll(transform, Input::Get().GetScrollOffset(), dt);
        }
    }

    void FPSCameraController::HandleKeyboard(Maths::Transform& transform, float dt)
    {
        m_CameraSpeed = 1000.0f * dt;

        if(Input::Get().GetKeyHeld(Lumos::InputCode::Key::W))
        {
            m_Velocity += transform.GetForwardDirection() * m_CameraSpeed;
        }

        if(Input::Get().GetKeyHeld(Lumos::InputCode::Key::S))
        {
            m_Velocity -= transform.GetForwardDirection() * m_CameraSpeed;
        }

        if(Input::Get().GetKeyHeld(Lumos::InputCode::Key::A))
        {
            m_Velocity -= transform.GetRightDirection() * m_CameraSpeed;
        }

        if(Input::Get().GetKeyHeld(Lumos::InputCode::Key::D))
        {
            m_Velocity += transform.GetRightDirection() * m_CameraSpeed;
        }

        if(Input::Get().GetKeyHeld(Lumos::InputCode::Key::Space))
        {
            m_Velocity -= transform.GetUpDirection() * m_CameraSpeed;
        }

        if(Input::Get().GetKeyHeld(Lumos::InputCode::Key::LeftShift))
        {
            m_Velocity += transform.GetUpDirection() * m_CameraSpeed;
        }

        if(glm::length(m_Velocity) > Maths::M_EPSILON)
        {
            glm::vec3 position = transform.GetLocalPosition();
            position += m_Velocity * dt;
            transform.SetLocalPosition(position);
            m_Velocity = m_Velocity * pow(m_DampeningFactor, dt);
        }
    }
}
