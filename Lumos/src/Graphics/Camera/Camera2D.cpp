#include "lmpch.h"
#include "Camera2D.h"
#include "App/Application.h"
#include "Core/OS/Input.h"
#include "Maths/Maths.h"

#include "Core/OS/Window.h"

#include <imgui/imgui.h>

namespace Lumos
{
    Camera2D::Camera2D(float aspectRatio, float scale) : Camera(45.0f, 0.0f, 1.0f, 1.0f)
		, m_AspectRatio(aspectRatio)
	{
		m_Orthographic = true;
		Application::Instance()->GetWindow()->HideMouse(false);
        m_Scale = scale;
		m_ProjMatrix = Maths::Matrix4::Orthographic(-m_AspectRatio * m_Scale, m_AspectRatio * m_Scale, -m_Scale, m_Scale, -1.0f, 1.0f);
		m_Position = Maths::Vector3(0.0f);
		m_Velocity = Maths::Vector3(0.0f);
		m_MouseSensitivity = 0.005f;
        BuildViewMatrix();
	}

	Camera2D::~Camera2D()
	{
	}

	void Camera2D::HandleMouse(float dt, float xpos, float ypos)
	{
		if (Input::GetInput()->GetMouseHeld(InputCode::MouseKey::ButtonRight))
		{
			m_Position.x -= (xpos - m_PreviousCurserPos.x) * m_Scale * m_MouseSensitivity * 0.5f;
			m_Position.y += (ypos - m_PreviousCurserPos.y) * m_Scale * m_MouseSensitivity * 0.5f;
		}

		m_PreviousCurserPos = Maths::Vector2(xpos, ypos);
	}

	void Camera2D::HandleKeyboard(float dt)
	{
		Maths::Vector3 up = Maths::Vector3(0, 1, 0), right = Maths::Vector3(1, 0, 0);

		m_CameraSpeed = m_Scale * dt * 20.0f;

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

		m_Position += m_Velocity * dt;
		m_Velocity = m_Velocity * pow(m_DampeningFactor, dt);

		UpdateScroll(Input::GetInput()->GetScrollOffset(), dt);
		Input::GetInput()->SetScrollOffset(0.0f);
	}

    void Camera2D::UpdateScroll(float offset, float dt)
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
        
        m_Scale -= m_ZoomVelocity;
        m_Scale = Lumos::Maths::Max(m_Scale, 0.15f);
        m_ZoomVelocity = m_ZoomVelocity * pow(m_ZoomDampeningFactor, dt);
    }
}
