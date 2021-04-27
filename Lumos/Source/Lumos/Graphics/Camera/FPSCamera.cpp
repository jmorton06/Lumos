#include "Precompiled.h"
#include "FPSCamera.h"
#include "Core/Application.h"
#include "Core/OS/Input.h"
#include "Core/OS/Window.h"
#include "Camera.h"

namespace Lumos
{

    FPSCameraController::FPSCameraController()
    {
    }

    FPSCameraController::~FPSCameraController() = default;

    void FPSCameraController::HandleMouse(Maths::Transform& transform, float dt, float xpos, float ypos)
    {
        if(Input::Get().GetWindowFocus())
        {
            {
                Maths::Vector2 windowCentre = Maths::Vector2();
                xpos -= windowCentre.x;
                ypos -= windowCentre.y;

                Application::Get().GetWindow()->SetMousePosition(windowCentre);

                Maths::Vector3 euler = transform.GetLocalOrientation().EulerAngles();
                float pitch = euler.x;
                float yaw = euler.y;

                pitch -= (ypos)*m_MouseSensitivity;
                yaw -= (xpos)*m_MouseSensitivity;

                transform.SetLocalOrientation(Maths::Quaternion::EulerAnglesToQuaternion(pitch, yaw, euler.z));
            }

            m_PreviousCurserPos = Maths::Vector2(xpos, ypos);

            UpdateScroll(transform, Input::Get().GetScrollOffset(), dt);
        }
    }

    void FPSCameraController::HandleKeyboard(Maths::Transform& transform, float dt)
    {
        Maths::Vector3 euler = transform.GetLocalOrientation().EulerAngles();
        float pitch = euler.x;
        float yaw = euler.y;

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

        if(!Maths::Equals(m_Velocity, Maths::Vector3::ZERO, Maths::Vector3(Maths::M_EPSILON)))
        {
            Maths::Vector3 position = transform.GetLocalPosition();
            position += m_Velocity * dt;
            transform.SetLocalPosition(position);
            m_Velocity = m_Velocity * pow(m_DampeningFactor, dt);
        }
    }
}
