#include "Precompiled.h"
#include "EditorCamera.h"
#include "Editor.h"
#include "Graphics/Camera/Camera.h"
#include "Core/Application.h"
#include "Core/OS/Input.h"

namespace Lumos
{
	EditorCameraController::EditorCameraController()
	{
		m_FocalPoint = Maths::Vector3::ZERO;
		m_Velocity = Maths::Vector3(0.0f);
		m_MouseSensitivity = 0.00001f;
        m_ZoomDampeningFactor = 0.00001f;
        m_DampeningFactor = 0.00001f;
        m_RotateDampeningFactor = 0.0000001f;
	}

	EditorCameraController::~EditorCameraController()
	{
	}

	void EditorCameraController::HandleMouse(Maths::Transform& transform, float dt, float xpos, float ypos)
	{
		LUMOS_PROFILE_FUNCTION();
        if(m_2DMode)
		{
			if(Input::GetInput()->GetMouseHeld(InputCode::MouseKey::ButtonRight))
			{
				m_MouseSensitivity = 0.0005f;
				Maths::Vector3 position = transform.GetLocalPosition();
				position.x -= (xpos - m_PreviousCurserPos.x) /** camera->GetScale() */* m_MouseSensitivity * 0.5f;
				position.y += (ypos - m_PreviousCurserPos.y) /** camera->GetScale() */ *m_MouseSensitivity * 0.5f;
				transform.SetLocalPosition(position);
			}
        }
        else
		{
			if(Input::GetInput()->GetMouseHeld(InputCode::MouseKey::ButtonRight))
			{
				m_MouseSensitivity = 0.02f;
				m_RotateVelocity = m_RotateVelocity + Maths::Vector2((xpos - m_PreviousCurserPos.x), (ypos - m_PreviousCurserPos.y)) * m_MouseSensitivity;
                //Application::Get().GetWindow()->HideMouse(true);
               // Application::Get().GetWindow()->SetMousePosition(m_PreviousCurserPos);
               // LUMOS_LOG_INFO(m_PreviousCurserPos);
               // LUMOS_LOG_INFO("{0}, {1}", xpos, ypos);
            }
            else
            {
                //Application::Get().GetWindow()->HideMouse(false);
            }
            
				
			Maths::Vector3 euler = transform.GetLocalOrientation().EulerAngles();
			float pitch = euler.x - m_RotateVelocity.y;
			float yaw = euler.y - m_RotateVelocity.x;
			
			pitch = Maths::Min(pitch, 84.0f);
			pitch = Maths::Max(pitch, -84.0f);
            m_PreviousCurserPos = Maths::Vector2(xpos, ypos);

			transform.SetLocalOrientation(Maths::Quaternion::EulerAnglesToQuaternion(pitch, yaw, 0.0f));
		}

		m_RotateVelocity = m_RotateVelocity * pow(m_RotateDampeningFactor, dt);

		UpdateScroll(transform, Input::GetInput()->GetScrollOffset(), dt);
	}

	void EditorCameraController::HandleKeyboard(Maths::Transform& transform, float dt)
	{
		if(m_2DMode)
		{
			Maths::Vector3 up = Maths::Vector3(0, 1, 0), right = Maths::Vector3(1, 0, 0);

			m_CameraSpeed = /*camera->GetScale() **/ dt * 20.0f;

			if(Input::GetInput()->GetKeyHeld(Lumos::InputCode::Key::A))
			{
				m_Velocity -= right * m_CameraSpeed;
			}

			if(Input::GetInput()->GetKeyHeld(Lumos::InputCode::Key::D))
			{
				m_Velocity += right * m_CameraSpeed;
			}

			if(Input::GetInput()->GetKeyHeld(Lumos::InputCode::Key::W))
			{
				m_Velocity += up * m_CameraSpeed;
			}

			if(Input::GetInput()->GetKeyHeld(Lumos::InputCode::Key::S))
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
		}
		else
		{

			float multiplier = 1000.0f;

			if(Input::GetInput()->GetKeyHeld(InputCode::Key::LeftShift))
			{
				multiplier = 10000.0f;
			}

			m_CameraSpeed = multiplier * dt;

			if(Input::GetInput()->GetMouseHeld(InputCode::MouseKey::ButtonRight))
			{
				if(Input::GetInput()->GetKeyHeld(InputCode::Key::W))
				{
					m_Velocity -= transform.GetForwardDirection() * m_CameraSpeed;
				}

				if(Input::GetInput()->GetKeyHeld(InputCode::Key::S))
				{
					m_Velocity += transform.GetForwardDirection() * m_CameraSpeed;
				}

				if(Input::GetInput()->GetKeyHeld(InputCode::Key::A))
				{
					m_Velocity -= transform.GetRightDirection() * m_CameraSpeed;
				}

				if(Input::GetInput()->GetKeyHeld(InputCode::Key::D))
				{
					m_Velocity += transform.GetRightDirection() * m_CameraSpeed;
				}

				if(Input::GetInput()->GetKeyHeld(InputCode::Key::Q))
				{
					m_Velocity -= transform.GetUpDirection() * m_CameraSpeed;
				}

				if(Input::GetInput()->GetKeyHeld(InputCode::Key::E))
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

	void EditorCameraController::UpdateScroll(Maths::Transform& transform, float offset, float dt)
	{
		if(m_2DMode)
		{
			float multiplier = 2.0f;
			if(Input::GetInput()->GetKeyHeld(InputCode::Key::LeftShift))
			{
				multiplier = 10.0f;
			}

			if(offset != 0.0f)
			{
				m_ZoomVelocity += dt * offset * multiplier;
			}

			if(!Maths::Equals(m_ZoomVelocity, 0.0f))
			{
				float scale = 1.0f;//camera->GetScale();

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

				//camera->SetScale(scale);
			}
		}
		else
		{

			if(offset != 0.0f)
			{
				m_ZoomVelocity -= dt * offset * 10.0f;
			}

			if(!Maths::Equals(m_ZoomVelocity, 0.0f))
			{
				Maths::Vector3 pos = transform.GetLocalPosition();
				pos += transform.GetForwardDirection() * m_ZoomVelocity;
				m_ZoomVelocity = m_ZoomVelocity * pow(m_ZoomDampeningFactor, dt);

				transform.SetLocalPosition(pos);
			}
		}
	}
}
