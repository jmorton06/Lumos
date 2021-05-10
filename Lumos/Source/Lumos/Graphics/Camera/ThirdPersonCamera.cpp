#include "Precompiled.h"
#include "ThirdPersonCamera.h"
#include "Core/OS/Input.h"
#include "Camera.h"

namespace Lumos
{

    ThirdPersonCameraController::ThirdPersonCameraController()
    {
        m_RotateDampeningFactor = 0.0f;
    }

    ThirdPersonCameraController::~ThirdPersonCameraController()
    {
    }

    void ThirdPersonCameraController::HandleMouse(Maths::Transform& transform, float dt, float xpos, float ypos)
    {
        if(Input::Get().GetMouseHeld(InputCode::MouseKey::ButtonRight))
        {
            {
                m_MouseSensitivity = 0.1f;
                m_RotateVelocity = m_RotateVelocity + Maths::Vector2((xpos - m_PreviousCurserPos.x), (ypos - m_PreviousCurserPos.y)) * m_MouseSensitivity;
                Maths::Vector3 euler = transform.GetLocalOrientation().EulerAngles();
                float pitch = euler.x;
                float yaw = euler.y;

                pitch -= m_RotateVelocity.y;
                yaw -= m_RotateVelocity.x;

                if(yaw < 0)
                {
                    yaw += 360.0f;
                }
                if(yaw > 360.0f)
                {
                    yaw -= 360.0f;
                }

                transform.SetLocalOrientation(Maths::Quaternion::EulerAnglesToQuaternion(pitch, yaw, euler.z));
            }
        }

        m_PreviousCurserPos = Maths::Vector2(xpos, ypos);

        m_RotateVelocity = m_RotateVelocity * pow(m_RotateDampeningFactor, dt);

        UpdateScroll(transform, Input::Get().GetScrollOffset(), dt);
    }

    void ThirdPersonCameraController::HandleKeyboard(Maths::Transform& transform, float dt)
    {
        float multiplier = 1000.0f;

        if(Input::Get().GetKeyHeld(InputCode::Key::LeftShift))
        {
            multiplier = 10000.0f;
        }

        m_CameraSpeed = multiplier * dt;

        if(Input::Get().GetMouseHeld(InputCode::MouseKey::ButtonRight))
        {
            if(Input::Get().GetKeyHeld(InputCode::Key::W))
            {
                m_Velocity -= transform.GetForwardDirection() * m_CameraSpeed;
            }

            if(Input::Get().GetKeyHeld(InputCode::Key::S))
            {
                m_Velocity += transform.GetForwardDirection() * m_CameraSpeed;
            }

            if(Input::Get().GetKeyHeld(InputCode::Key::A))
            {
                m_Velocity -= transform.GetRightDirection() * m_CameraSpeed;
            }

            if(Input::Get().GetKeyHeld(InputCode::Key::D))
            {
                m_Velocity += transform.GetRightDirection() * m_CameraSpeed;
            }

            if(Input::Get().GetKeyHeld(InputCode::Key::Q))
            {
                m_Velocity -= transform.GetUpDirection() * m_CameraSpeed;
            }

            if(Input::Get().GetKeyHeld(InputCode::Key::E))
            {
                m_Velocity += transform.GetUpDirection() * m_CameraSpeed;
            }
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
