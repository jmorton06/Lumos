#include "LM.h"
#include "ThirdPersonCamera.h"
#include "App/Application.h"
#include "App/Input.h"
#include "App/Window.h"

namespace Lumos
{

	ThirdPersonCamera::ThirdPersonCamera(float FOV, float Near, float Far, float aspect)
		: Camera(FOV, Near, Far, aspect), m_Free(false)
	{
		Application::Instance()->GetWindow()->HideMouse(false);
		m_ProjMatrix = Maths::Matrix4::Perspective(m_Near, m_Far, aspect, FOV);
		m_RotateDampeningFactor = 0.0f;
	}

	ThirdPersonCamera::ThirdPersonCamera(float pitch, float yaw, const Maths::Vector3& position, float FOV, float Near, float Far, float aspect)
		: Camera(pitch, yaw, position, FOV, Near, Far, aspect), m_Free(false)
	{
		Application::Instance()->GetWindow()->HideMouse(false);
		m_ProjMatrix = Maths::Matrix4::Perspective(m_Near, m_Far, aspect, FOV);
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
            if (Input::GetInput().GetMouseHeld(InputCode::MouseKey::ButtonRight))
			{
				m_RotateVelocity = m_RotateVelocity + Maths::Vector2((xpos - m_PreviousCurserPos.GetX()), (ypos - m_PreviousCurserPos.GetY())) *  m_MouseSensitivity;
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

			m_PreviousCurserPos = Maths::Vector2(xpos, ypos);
		}

		m_RotateVelocity = m_RotateVelocity * pow(m_RotateDampeningFactor, dt);
		UpdateScroll(Input::GetInput().GetScrollOffset(), dt);
		Input::GetInput().SetScrollOffset(0.0f);
	}

	void ThirdPersonCamera::HandleKeyboard(float dt)
	{
		m_CameraSpeed = 1000.0f * dt;

        if (Input::GetInput().GetKeyHeld(InputCode::Key::W))
		{
			m_Velocity += GetForwardDirection() * m_CameraSpeed;
		}

        if (Input::GetInput().GetKeyHeld(InputCode::Key::S))
		{
			m_Velocity -= GetForwardDirection() * m_CameraSpeed;
		}

        if (Input::GetInput().GetKeyHeld(InputCode::Key::A))
		{
			m_Velocity -= GetRightDirection() * m_CameraSpeed;
		}

        if (Input::GetInput().GetKeyHeld(InputCode::Key::D))
		{
			m_Velocity += GetRightDirection() * m_CameraSpeed;
		}

        if (Input::GetInput().GetKeyHeld(InputCode::Key::Space))
		{
			m_Velocity -= GetUpDirection() * m_CameraSpeed;
		}

        if (Input::GetInput().GetKeyHeld(InputCode::Key::LeftShift))
		{
			m_Velocity += GetUpDirection() * m_CameraSpeed;
		}

		m_Position += m_Velocity * dt;
		m_Velocity = m_Velocity * pow(m_DampeningFactor, dt);
	}
}
