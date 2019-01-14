#include "JM.h"
#include "ThirdPersonCamera.h"
#include "App/Application.h"

namespace jm
{

	ThirdPersonCamera::ThirdPersonCamera(float FOV, float Near, float Far, float aspect)
		: Camera(FOV, Near, Far, aspect)
	{
		Application::Instance()->GetWindow()->HideMouse(false);
		m_ProjMatrix = maths::Matrix4::Perspective(m_Near, m_Far, aspect, FOV);
		m_RotateDampeningFactor = 0.0f;
	}

	ThirdPersonCamera::ThirdPersonCamera(float pitch, float yaw, const maths::Vector3& position, float FOV, float Near, float Far, float aspect)
		: Camera(pitch, yaw, position, FOV, Near, Far, aspect)
	{
		Application::Instance()->GetWindow()->HideMouse(false);
		m_ProjMatrix = maths::Matrix4::Perspective(m_Near, m_Far, aspect, FOV);
		m_RotateDampeningFactor = 0.0f;
	}

	ThirdPersonCamera::~ThirdPersonCamera()
	{
	}

	void ThirdPersonCamera::HandleMouse(float dt, float xpos, float ypos)
	{
		//if (Input::GetInput().m_UpdateCamera)
		{
			//if (!Input::GetInput().firstUpdate)
			if (Input::GetInput().GetMouseHeld(JM_MOUSE_RIGHT))
			{
				m_RotateVelocity = m_RotateVelocity + maths::Vector2((xpos - m_PreviousCurserPos.GetX()), (ypos - m_PreviousCurserPos.GetY())) *  m_MouseSensitivity;
				m_Pitch -= m_RotateVelocity.GetY();
				m_Yaw -= m_RotateVelocity.GetX();

				//m_Pitch = Maths::Min(m_Pitch, 90.0f);
				//m_Pitch = Maths::Max(m_Pitch, -90.0f);

				if (m_Yaw < 0)
				{
					m_Yaw += 360.0f;
				}
				if (m_Yaw > 360.0f)
				{
					m_Yaw -= 360.0f;
				}
			}
			/*else
			{
			Input::GetInput().firstUpdate = false;
			}*/

			m_PreviousCurserPos = maths::Vector2(xpos, ypos);
		}

		m_RotateVelocity = m_RotateVelocity * pow(m_RotateDampeningFactor, dt);
		UpdateScroll(Input::GetInput().GetScrollOffset(), dt);
		Input::GetInput().SetScrollOffset(0.0f);
	}

	void ThirdPersonCamera::HandleKeyboard(float dt)
	{
		m_CameraSpeed = 1000.0f * dt;

		if (Input::GetInput().GetKeyHeld(JM_KEY_W))
		{
			m_Velocity += GetForwardDirection() * m_CameraSpeed;
		}

		if (Input::GetInput().GetKeyHeld(JM_KEY_S))
		{
			m_Velocity -= GetForwardDirection() * m_CameraSpeed;
		}

		if (Input::GetInput().GetKeyHeld(JM_KEY_A))
		{
			m_Velocity -= GetRightDirection() * m_CameraSpeed;
		}

		if (Input::GetInput().GetKeyHeld(JM_KEY_D))
		{
			m_Velocity += GetRightDirection() * m_CameraSpeed;
		}

		if (Input::GetInput().GetKeyHeld(JM_KEY_SPACE))
		{
			m_Velocity -= GetUpDirection() * m_CameraSpeed;
		}

		if (Input::GetInput().GetKeyHeld(JM_KEY_LEFT_SHIFT))
		{
			m_Velocity += GetUpDirection() * m_CameraSpeed;
		}

		m_Position += m_Velocity * dt;
		m_Velocity = m_Velocity * pow(m_DampeningFactor, dt);
	}
}
