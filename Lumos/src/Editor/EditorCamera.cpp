#include "lmpch.h"
#include "EditorCamera.h"
#include "Graphics/Camera/Camera.h"

#include "Core/OS/Input.h"

namespace Lumos
{
	EditorCameraController::EditorCameraController(Camera* camera)
		: CameraController(camera)
	{
		m_RotateDampeningFactor = 0.0f;
		m_FocalPoint = Maths::Vector3::ZERO;
	}

	EditorCameraController::~EditorCameraController()
	{
	}

	void EditorCameraController::HandleMouse(float dt, float xpos, float ypos)
	{
		if (Input::GetInput()->GetMouseHeld(InputCode::MouseKey::ButtonRight))
		{
			m_RotateVelocity = m_RotateVelocity + Maths::Vector2((xpos - m_PreviousCurserPos.x), (ypos - m_PreviousCurserPos.y)) *  m_MouseSensitivity;
        
            float pitch = m_Camera->GetPitch();
            float yaw = m_Camera->GetYaw();
        
			pitch -= m_RotateVelocity.y;
			yaw -= m_RotateVelocity.x;

			if (yaw < 0)
			{
				yaw += 360.0f;
			}
			if (yaw > 360.0f)
			{
				yaw -= 360.0f;
			}
        
            m_Camera->SetYaw(yaw);
            m_Camera->SetPitch(pitch);
		}

		m_PreviousCurserPos = Maths::Vector2(xpos, ypos);

		m_RotateVelocity = m_RotateVelocity * pow(m_RotateDampeningFactor, dt);

		UpdateScroll(Input::GetInput()->GetScrollOffset(), dt);
	}

	void EditorCameraController::HandleKeyboard(float dt)
	{
		float multiplier = 1000.0f;

		if (Input::GetInput()->GetKeyHeld(InputCode::Key::LeftShift))
		{
			multiplier = 10000.0f;
		}

		m_CameraSpeed = multiplier * dt;

		if (Input::GetInput()->GetMouseHeld(InputCode::MouseKey::ButtonRight))
		{        
			if (Input::GetInput()->GetKeyHeld(InputCode::Key::W))
			{
				m_Velocity -= m_Camera->GetForwardDirection() * m_CameraSpeed;
			}

			if (Input::GetInput()->GetKeyHeld(InputCode::Key::S))
			{
				m_Velocity += m_Camera->GetForwardDirection() * m_CameraSpeed;
			}

			if (Input::GetInput()->GetKeyHeld(InputCode::Key::A))
			{
				m_Velocity -= m_Camera->GetRightDirection() * m_CameraSpeed;
			}

			if (Input::GetInput()->GetKeyHeld(InputCode::Key::D))
			{
				m_Velocity += m_Camera->GetRightDirection() * m_CameraSpeed;
			}
		}
		
		if (Input::GetInput()->GetKeyHeld(InputCode::Key::Q))
		{
			m_Velocity -= m_Camera->GetUpDirection() * m_CameraSpeed;
		}

		if (Input::GetInput()->GetKeyHeld(InputCode::Key::E))
		{
			m_Velocity += m_Camera->GetUpDirection() * m_CameraSpeed;
		}

		if (!Maths::Equals(m_Velocity, Maths::Vector3::ZERO, Maths::Vector3(Maths::M_EPSILON)))
		{
            Maths::Vector3 position = m_Camera->GetPosition();
			position += m_Velocity * dt;
            m_Camera->SetPosition(position);
			m_Velocity = m_Velocity * pow(m_DampeningFactor, dt);
		}

		if (Input::GetInput()->GetKeyHeld(InputCode::Key::F))
		{
			//Focus on m_SelectedEntity
		}

		if (Input::GetInput()->GetKeyHeld(InputCode::Key::O))
		{
			//Focus on origin
		}
	}
    
    void EditorCameraController::UpdateScroll(float offset, float dt)
    {
        if (offset != 0.0f)
        {
            m_ZoomVelocity -= dt * offset * 10.0f;
        }

        if (!Maths::Equals(m_ZoomVelocity, 0.0f))
        {
            Maths::Vector3 pos = m_Camera->GetPosition();
            pos += m_Camera->GetForwardDirection() * m_ZoomVelocity;
            m_ZoomVelocity = m_ZoomVelocity * pow(m_ZoomDampeningFactor, dt);
            
            m_Camera->SetPosition(pos);
        }
    }
}
