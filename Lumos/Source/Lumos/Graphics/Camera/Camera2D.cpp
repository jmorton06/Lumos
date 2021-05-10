#include "Precompiled.h"
#include "Camera2D.h"
#include "Core/OS/Input.h"
#include "Maths/Maths.h"
#include "Camera.h"

namespace Lumos
{
    CameraController2D::CameraController2D()
    {
        m_Velocity = Maths::Vector3(0.0f);
        m_MouseSensitivity = 0.005f;
    }

    CameraController2D::~CameraController2D() = default;

    void CameraController2D::HandleMouse(Maths::Transform& transform, float dt, float xpos, float ypos)
    {
        if(Input::Get().GetMouseHeld(InputCode::MouseKey::ButtonRight))
        {
            Maths::Vector3 position = transform.GetLocalPosition();
            position.x -= (xpos - m_PreviousCurserPos.x) * m_MouseSensitivity * 0.5f;
            position.y += (ypos - m_PreviousCurserPos.y) * m_MouseSensitivity * 0.5f;
            transform.SetLocalPosition(position);
        }

        m_PreviousCurserPos = Maths::Vector2(xpos, ypos);
    }

    void CameraController2D::HandleKeyboard(Maths::Transform& transform, float dt)
    {
        Maths::Vector3 up = Maths::Vector3(0, 1, 0), right = Maths::Vector3(1, 0, 0);

        m_CameraSpeed = dt * 20.0f; //camera->GetScale() *

        if(Input::Get().GetKeyHeld(Lumos::InputCode::Key::A))
        {
            m_Velocity -= right * m_CameraSpeed;
        }

        if(Input::Get().GetKeyHeld(Lumos::InputCode::Key::D))
        {
            m_Velocity += right * m_CameraSpeed;
        }

        if(Input::Get().GetKeyHeld(Lumos::InputCode::Key::W))
        {
            m_Velocity += up * m_CameraSpeed;
        }

        if(Input::Get().GetKeyHeld(Lumos::InputCode::Key::S))
        {
            m_Velocity -= up * m_CameraSpeed;
        }

        if(!Maths::Equals(m_Velocity, Maths::Vector3::ZERO, Maths::Vector3(Maths::M_EPSILON)))
        {
            Maths::Vector3 position = transform.GetLocalPosition();
            position += m_Velocity * dt;
            m_Velocity = m_Velocity * pow(m_DampeningFactor, dt);

            transform.SetLocalPosition(position);
        }

        UpdateScroll(transform, Input::Get().GetScrollOffset(), dt);
    }

    void CameraController2D::UpdateScroll(Maths::Transform& transform, float offset, float dt)
    {
        float multiplier = 2.0f;
        if(Input::Get().GetKeyHeld(InputCode::Key::LeftShift))
        {
            multiplier = 10.0f;
        }

        if(offset != 0.0f)
        {
            m_ZoomVelocity += dt * offset * multiplier;
        }

        if(!Maths::Equals(m_ZoomVelocity, 0.0f))
        {
            float scale = 1.0f; //camera->GetScale();

            scale -= m_ZoomVelocity;

            if(scale < 0.15f)
            {
                scale = 0.15f;
                m_ZoomVelocity = 0.0f;
            }
            else
            {
                m_ZoomVelocity = m_ZoomVelocity * pow(m_ZoomDampeningFactor, dt);
            }

            //camera->SetScale(scale); TODO
        }
    }
}
