#include "lmpch.h"
#include "EditorCamera.h"

#include "Core/OS/Input.h"
#include "Core/OS/Window.h"

namespace Lumos
{
	EditorCamera::EditorCamera(float pitch, float yaw, const Maths::Vector3& position, float FOV, float Near, float Far, float aspect)
		: Camera(pitch, yaw, position, FOV, Near, Far, aspect)
	{
		m_RotateDampeningFactor = 0.0f;
		m_FocalPoint = Maths::Vector3::ZERO;
	}

	EditorCamera::~EditorCamera()
	{
	}

	void EditorCamera::HandleMouse(float dt, float xpos, float ypos)
	{
		if (Input::GetInput()->GetMouseHeld(InputCode::MouseKey::ButtonRight))
		{
			m_RotateVelocity = m_RotateVelocity + Maths::Vector2((xpos - m_PreviousCurserPos.x), (ypos - m_PreviousCurserPos.y)) *  m_MouseSensitivity;
			m_Pitch -= m_RotateVelocity.y;
			m_Yaw -= m_RotateVelocity.x;

			if (m_Yaw < 0)
			{
				m_Yaw += 360.0f;
			}
			if (m_Yaw > 360.0f)
			{
				m_Yaw -= 360.0f;
			}
		}

		m_PreviousCurserPos = Maths::Vector2(xpos, ypos);

		m_RotateVelocity = m_RotateVelocity * pow(m_RotateDampeningFactor, dt);

		m_ViewDirty = true;
		m_FrustumDirty = true;

		UpdateScroll(Input::GetInput()->GetScrollOffset(), dt);
	}

	void EditorCamera::HandleKeyboard(float dt)
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
				m_Velocity -= GetForwardDirection() * m_CameraSpeed;
			}

			if (Input::GetInput()->GetKeyHeld(InputCode::Key::S))
			{
				m_Velocity += GetForwardDirection() * m_CameraSpeed;
			}

			if (Input::GetInput()->GetKeyHeld(InputCode::Key::A))
			{
				m_Velocity -= GetRightDirection() * m_CameraSpeed;
			}

			if (Input::GetInput()->GetKeyHeld(InputCode::Key::D))
			{
				m_Velocity += GetRightDirection() * m_CameraSpeed;
			}
		}
		
		if (Input::GetInput()->GetKeyHeld(InputCode::Key::Q))
		{
			m_Velocity -= GetUpDirection() * m_CameraSpeed;
		}

		if (Input::GetInput()->GetKeyHeld(InputCode::Key::E))
		{
			m_Velocity += GetUpDirection() * m_CameraSpeed;
		}

		if (!Maths::Equals(m_Velocity, Maths::Vector3::ZERO, Maths::Vector3(Maths::M_EPSILON)))
		{
			m_Position += m_Velocity * dt;
			m_Velocity = m_Velocity * pow(m_DampeningFactor, dt);
			m_ViewDirty = true;
			m_FrustumDirty = true;
		}

		if (Input::GetInput()->GetKeyHeld(InputCode::Key::F))
		{
			//Focus on m_SelectedEntity
		}

		if (Input::GetInput()->GetKeyHeld(InputCode::Key::O))
		{
			//Focus on origin
		}

		m_ViewDirty = true;
	}
}
