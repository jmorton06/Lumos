#include "lmpch.h"
#include "ThirdPersonCamera.h"
#include "Core/OS/Input.h"
#include "Camera.h"
#include <imgui/imgui.h>

namespace Lumos
{

	ThirdPersonCameraController::ThirdPersonCameraController(Camera* camera)
		: CameraController(camera)
	{
		m_RotateDampeningFactor = 0.0f;
	}

	ThirdPersonCameraController::~ThirdPersonCameraController()
	{
	}

	void ThirdPersonCameraController::HandleMouse(float dt, float xpos, float ypos)
	{
		{
            if (Input::GetInput()->GetMouseHeld(InputCode::MouseKey::ButtonRight))
			{
                float pitch = m_Camera->GetPitch();
                float yaw = m_Camera->GetYaw();
            
				m_RotateVelocity = m_RotateVelocity + Maths::Vector2((xpos - m_PreviousCurserPos.x), (ypos - m_PreviousCurserPos.y)) *  m_MouseSensitivity;
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
            
                m_Camera->SetPitch(pitch);
                m_Camera->SetYaw(yaw);
			}

			m_PreviousCurserPos = Maths::Vector2(xpos, ypos);
		}

		m_RotateVelocity = m_RotateVelocity * pow(m_RotateDampeningFactor, dt);
		UpdateScroll(Input::GetInput()->GetScrollOffset(), dt);
	}

	void ThirdPersonCameraController::HandleKeyboard(float dt)
	{
		m_CameraSpeed = 1000.0f * dt;
        
        Maths::Vector3 m_Velocity;

        if (Input::GetInput()->GetKeyHeld(InputCode::Key::W))
		{
			m_Velocity += m_Camera->GetForwardDirection() * m_CameraSpeed;
		}

        if (Input::GetInput()->GetKeyHeld(InputCode::Key::S))
		{
			m_Velocity -= m_Camera->GetForwardDirection() * m_CameraSpeed;
		}

        if (Input::GetInput()->GetKeyHeld(InputCode::Key::A))
		{
			m_Velocity -= m_Camera->GetRightDirection() * m_CameraSpeed;
		}

        if (Input::GetInput()->GetKeyHeld(InputCode::Key::D))
		{
			m_Velocity += m_Camera->GetRightDirection() * m_CameraSpeed;
		}

        if (Input::GetInput()->GetKeyHeld(InputCode::Key::Space))
		{
			m_Velocity -= m_Camera->GetUpDirection() * m_CameraSpeed;
		}

        if (Input::GetInput()->GetKeyHeld(InputCode::Key::LeftShift))
		{
			m_Velocity += m_Camera->GetUpDirection() * m_CameraSpeed;
		}

        Maths::Vector3 position = m_Camera->GetPosition();
    
		position += m_Velocity * dt;
		m_Velocity = m_Velocity * pow(m_DampeningFactor, dt);
    
        m_Camera->SetPosition(position);
	}
}
