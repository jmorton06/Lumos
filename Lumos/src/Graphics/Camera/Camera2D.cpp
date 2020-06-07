#include "lmpch.h"
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

    void CameraController2D::HandleMouse(Camera* camera, float dt, float xpos, float ypos)
	{
		if (Input::GetInput()->GetMouseHeld(InputCode::MouseKey::ButtonRight))
		{
            Maths::Vector3 position = camera->GetPosition();
			position.x -= (xpos - m_PreviousCurserPos.x) * camera->GetScale() * m_MouseSensitivity * 0.5f;
			position.y += (ypos - m_PreviousCurserPos.y) * camera->GetScale() * m_MouseSensitivity * 0.5f;
            camera->SetPosition(position);
		}

		m_PreviousCurserPos = Maths::Vector2(xpos, ypos);
	}

	void CameraController2D::HandleKeyboard(Camera* camera, float dt)
	{
		Maths::Vector3 up = Maths::Vector3(0, 1, 0), right = Maths::Vector3(1, 0, 0);

		m_CameraSpeed = camera->GetScale() * dt * 20.0f;

		if (Input::GetInput()->GetKeyHeld(LUMOS_KEY_A))
		{
			m_Velocity -= right * m_CameraSpeed;
		}

		if (Input::GetInput()->GetKeyHeld(LUMOS_KEY_D))
		{
			m_Velocity += right * m_CameraSpeed;
		}

		if (Input::GetInput()->GetKeyHeld(LUMOS_KEY_W))
		{
			m_Velocity += up * m_CameraSpeed;
		}

		if (Input::GetInput()->GetKeyHeld(LUMOS_KEY_S))
		{
			m_Velocity -= up * m_CameraSpeed;
		}

		if (!Maths::Equals(m_Velocity, Maths::Vector3::ZERO, Maths::Vector3(Maths::M_EPSILON)))
		{
            Maths::Vector3 position = camera->GetPosition();
			position += m_Velocity * dt;
			m_Velocity = m_Velocity * pow(m_DampeningFactor, dt);
        
            camera->SetPosition(position);
		}

		UpdateScroll(camera, Input::GetInput()->GetScrollOffset(), dt);
	}

    void CameraController2D::UpdateScroll(Camera* camera, float offset, float dt)
    {
		float multiplier = 2.0f;
		if (Input::GetInput()->GetKeyHeld(InputCode::Key::LeftShift))
		{
			multiplier = 10.0f;
		}

        if (offset != 0.0f)
        {
            m_ZoomVelocity += dt * offset * multiplier;
        }
        
		if (!Maths::Equals(m_ZoomVelocity, 0.0f))
		{
            float scale = camera->GetScale();

			scale -= m_ZoomVelocity;

			if (scale < 0.15f)
			{
				scale = 0.15f;
				m_ZoomVelocity = 0.0f;
			}
			else
			{
				m_ZoomVelocity = m_ZoomVelocity * pow(m_ZoomDampeningFactor, dt);
			}
        
            camera->SetScale(scale);
		}
    }
}
