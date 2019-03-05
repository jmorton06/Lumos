#include "LM.h"
#include "Camera2D.h"
#include "App/Application.h"
#include "Maths/MathsUtilities.h"

namespace Lumos
{
    Camera2D::Camera2D(uint width, uint height, float scale) : Camera(45.0f, 0.0f, 1.0f, 1.0f)
		, m_Scale(scale)
	{
		Application::Instance()->GetWindow()->HideMouse(false);
		m_ProjMatrix = maths::Matrix4::Orthographic(-1.0f, 1.0f, static_cast<float>(width), 0.0f, static_cast<float>(height), 0.0f);
		m_Position = maths::Vector3(0.0f);
		m_Velocity = maths::Vector3(0.0f);

        BuildViewMatrix();
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

		m_CameraSpeed = 100.0f * dt * m_Scale;

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
        
        UpdateScroll(Input::GetInput().GetScrollOffset(), dt);
		Input::GetInput().SetScrollOffset(0.0f);
	}

	void Camera2D::UpdateProjectionMatrix(float width, float height)
	{
		m_ProjMatrix = maths::Matrix4::Orthographic(-1.0f, 1.0f, width, 0.0f, height, 0.0f);
	}

	void Camera2D::BuildViewMatrix()
	{
		m_ViewMatrix = maths::Matrix4::Scale(maths::Vector3(m_Scale)) *
				maths::Matrix4::Translation(-m_Position);
	}

	float Camera2D::GetScale() const
	{
		return m_Scale;
	}

	void Camera2D::SetScale(float scale)
	{
		m_Scale = scale;
	}
    
    void Camera2D::UpdateScroll(float offset, float dt)
    {
        if (offset != 0.0f)
        {
            m_ZoomVelocity += dt * offset * 0.1f;
        }
        
        m_Scale += m_ZoomVelocity;
        m_Scale = maths::Max(m_Scale, 0.0f);
        m_ZoomVelocity = m_ZoomVelocity * pow(m_ZoomDampeningFactor, dt);
    }
}
