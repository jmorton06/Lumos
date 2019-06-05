#include "LM.h"
#include "FPSCamera.h"
#include "App/Application.h"
#include "App/Input.h"
#include "App/Window.h"

namespace Lumos
{

	FPSCamera::FPSCamera(float FOV, float Near, float Far, float aspect)
		: Camera(FOV, Near, Far, aspect)
	{
		Application::Instance()->GetWindow()->HideMouse(true);
		m_ProjMatrix = Maths::Matrix4::Perspective(m_Near, m_Far, aspect, FOV);
	}

	FPSCamera::FPSCamera(float pitch, float yaw, const Maths::Vector3& position, float FOV, float Near, float Far, float aspect)
		: Camera(pitch, yaw, position, FOV, Near, Far, aspect)
	{
		Application::Instance()->GetWindow()->HideMouse(true);
		m_ProjMatrix = Maths::Matrix4::Perspective(m_Near, m_Far, aspect, FOV);
	}

	FPSCamera::~FPSCamera() = default;

	void FPSCamera::HandleMouse(float dt, float xpos, float ypos)
	{
		if (Input::GetInput().GetWindowFocus())
		{
			//if (!Input::GetInput().firstUpdate)
			{
				//LUMOS_CORE_ASSERT(false,"");
				Maths::Vector2 windowCentre = Maths::Vector2();// Window::Instance()->GetScreenSize().GetX() / 2.0f, Window::Instance()->GetScreenSize().GetY() / 2.0f);
				xpos -= windowCentre.GetX();
				ypos -= windowCentre.GetY();

				m_Pitch -= (ypos)* m_MouseSensitivity;
				m_Yaw   -= (xpos)* m_MouseSensitivity;

				Application::Instance()->GetWindow()->SetMousePosition(windowCentre);

				if (m_Yaw < 0)
					m_Yaw += 360.0f;

				if (m_Yaw > 360.0f)
					m_Yaw -= 360.0f;
			}

			m_PreviousCurserPos = Maths::Vector2(xpos, ypos);
		}

		UpdateScroll(Input::GetInput().GetScrollOffset(), dt);
		Input::GetInput().SetScrollOffset(0.0f);
	}

	void FPSCamera::HandleKeyboard(float dt)
	{
		const Maths::Quaternion orientation = Maths::Quaternion::EulerAnglesToQuaternion(m_Pitch, m_Yaw, 1.0f);
		Maths::Vector3 up = Maths::Vector3(0, 1, 0), right = Maths::Vector3(1, 0, 0), forward = Maths::Vector3(0, 0, -1);
		Maths::Quaternion::RotatePointByQuaternion(orientation, up);
		Maths::Quaternion::RotatePointByQuaternion(orientation, right);
		Maths::Quaternion::RotatePointByQuaternion(orientation, forward);

		m_CameraSpeed = 1000.0f * dt;

		if (Input::GetInput().GetKeyHeld(LUMOS_KEY_W))
		{
			m_Velocity += forward * m_CameraSpeed;
		}

		if (Input::GetInput().GetKeyHeld(LUMOS_KEY_S))
		{
			m_Velocity -= forward * m_CameraSpeed;
		}

		if (Input::GetInput().GetKeyHeld(LUMOS_KEY_A))
		{
			m_Velocity -= right * m_CameraSpeed;
		}

		if (Input::GetInput().GetKeyHeld(LUMOS_KEY_D))
		{
			m_Velocity += right * m_CameraSpeed;
		}

		if (Input::GetInput().GetKeyHeld(LUMOS_KEY_SPACE))
		{
			m_Velocity -= up * m_CameraSpeed;
		}

		if (Input::GetInput().GetKeyHeld(LUMOS_KEY_LEFT_SHIFT))
		{
			m_Velocity += up * m_CameraSpeed;
		}

		m_Position += m_Velocity * dt;
		m_Velocity = m_Velocity * pow(m_DampeningFactor, dt);

	}

}
