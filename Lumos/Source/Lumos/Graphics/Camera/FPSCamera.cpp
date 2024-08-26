#include "Precompiled.h"
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
                Vec2 windowCentre = Vec2();
                xpos -= windowCentre.x;
                ypos -= windowCentre.y;

                Application::Get().GetWindow()->SetMousePosition(windowCentre);

                Vec3 euler  = transform.GetLocalOrientation().ToEuler();
                float pitch = euler.x;
                float yaw   = euler.y;

                pitch -= (ypos)*m_MouseSensitivity;
                yaw -= (xpos)*m_MouseSensitivity;

                transform.SetLocalOrientation(Quat(Vec3(pitch, yaw, euler.z)));
            }

            m_PreviousCurserPos = Vec2(xpos, ypos);

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

        if(Maths::Length(m_Velocity) > Maths::M_EPSILON)
        {
            Vec3 position = transform.GetLocalPosition();
            position += m_Velocity * dt;
            transform.SetLocalPosition(position);
            m_Velocity = m_Velocity * pow(m_DampeningFactor, dt);
        }
    }
}
