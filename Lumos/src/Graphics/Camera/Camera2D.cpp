#include "JM.h"
#include "Camera2D.h"
#include "App/Application.h"

namespace jm
{

	Camera2D::Camera2D(float FOV, float Near, float Far, float aspect, int scale)
		: Camera(FOV, Near, Far, aspect)
		, m_Scale(scale)
	{
		Application::Instance()->GetWindow()->HideMouse(false);
		m_ProjMatrix = maths::Matrix4::Orthographic(-1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f);
		m_Position = maths::Vector3(0.0f);
	}

	Camera2D::Camera2D(uint width, uint height, float aspect, int scale) : Camera(45.0f, 0.0f, 1.0f, aspect)
		, m_Scale(scale)
	{
		Application::Instance()->GetWindow()->HideMouse(false);
		m_ProjMatrix = maths::Matrix4::Orthographic(-1.0f, 1.0f, static_cast<float>(width), 0.0f, static_cast<float>(height), 0.0f);
		m_Position = maths::Vector3(0.0f);

	}

	Camera2D::Camera2D(float pitch, float yaw, const maths::Vector3& position, float FOV, float Near, float Far, float aspect, int scale)
		: Camera(pitch, yaw, position, FOV, Near, Far, aspect)
		, m_Scale(scale)
	{
		Application::Instance()->GetWindow()->HideMouse(false);
		m_ProjMatrix = maths::Matrix4::Orthographic(-1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f);
		m_Position = maths::Vector3(0.0f);
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

		m_CameraSpeed = 1000.0f * dt;

		if (Input::GetInput().GetKeyHeld(JM_KEY_A))
		{
			m_Velocity -= right * m_CameraSpeed;
		}

		if (Input::GetInput().GetKeyHeld(JM_KEY_D))
		{
			m_Velocity += right * m_CameraSpeed;
		}

		if (Input::GetInput().GetKeyHeld(JM_KEY_W))
		{
			m_Velocity -= up * m_CameraSpeed;
		}

		if (Input::GetInput().GetKeyHeld(JM_KEY_S))
		{
			m_Velocity += up * m_CameraSpeed;
		}

		m_Position += m_Velocity * dt;
		m_Velocity = m_Velocity * pow(m_DampeningFactor, dt);
	}

	void Camera2D::UpdateProjectionMatrix(float width, float height)
	{
		m_ProjMatrix = maths::Matrix4::Orthographic(-1.0f, 1.0f, width, 0.0f, height, 0.0f);
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
