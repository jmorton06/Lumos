#include "LM.h"
#include "Camera2D.h"
#include "App/Application.h"

namespace Lumos
{
	Camera2D::Camera2D(uint width, uint height, int scale) : Camera(45.0f, 0.0f, 1.0f, 1.0f)
		, m_Scale(scale)
	{
		Application::Instance()->GetWindow()->HideMouse(false);
		m_ProjMatrix = maths::Matrix4::Orthographic(-1.0f, 1.0f, static_cast<float>(width), static_cast<float>(-width), static_cast<float>(height), static_cast<float>(-height));
		m_Position = maths::Vector3(0.0f);
		m_Velocity = maths::Vector3(0.0f);

	}

	Camera2D::~Camera2D()
	{
	}

	void Camera2D::HandleMouse(float dt, float xpos, float ypos)
	{
	}

	void Camera2D::HandleKeyboard(float dt)
	{
		maths::Vector3 up = maths::Vector3(0, 1, 0), right = maths::Vector3(1, 0, 0);

		m_CameraSpeed = 100.0f * dt;

		if (Input::GetInput().GetKeyHeld(LUMOS_KEY_A))
		{
			m_Velocity -= right * m_CameraSpeed;
		}

		if (Input::GetInput().GetKeyHeld(LUMOS_KEY_D))
		{
			m_Velocity += right * m_CameraSpeed;
		}

		if (Input::GetInput().GetKeyHeld(LUMOS_KEY_W))
		{
			m_Velocity -= up * m_CameraSpeed;
		}

		if (Input::GetInput().GetKeyHeld(LUMOS_KEY_S))
		{
			m_Velocity += up * m_CameraSpeed;
		}

		m_Position += m_Velocity * dt;
		m_Velocity = m_Velocity * pow(m_DampeningFactor, dt);
	}

	void Camera2D::UpdateProjectionMatrix(float width, float height)
	{
		m_ProjMatrix = maths::Matrix4::Orthographic(-1.0f, 1.0f, width, -width, height, -height);
	}

	void Camera2D::BuildViewMatrix()
	{
		m_ViewMatrix = maths::Matrix4::Scale(maths::Vector3(static_cast<float>(m_Scale))) *
				maths::Matrix4::Translation(-m_Position);
	}

	int Camera2D::GetScale() const
	{
		return m_Scale;
	}

	void Camera2D::SetScale(int scale)
	{
		m_Scale = scale;
	}
}
