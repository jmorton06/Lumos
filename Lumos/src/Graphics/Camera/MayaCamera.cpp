#include "LM.h"
#include "MayaCamera.h"
#include "App/Application.h"

namespace lumos
{

	MayaCamera::MayaCamera(float FOV, float Near, float Far, float aspect)
		: Camera(FOV, Near, Far, aspect), m_Free(false)
	{
		Application::Instance()->GetWindow()->HideMouse(false);

		m_PanSpeed = 0.002f;
		m_RotationSpeed = 1.2f;
		m_ZoomSpeed = 0.6f;

		m_Position = maths::Vector3(-3.0f, 10.0f, 15.0f);

		m_FocalPoint = maths::Vector3::Zero();
		maths::Vector3 distance = (m_Position - m_FocalPoint);
		m_Distance = distance.Length();// m_Position.Distance(m_FocalPoint);

		m_ProjMatrix = maths::Matrix4::Perspective(m_Near, m_Far, aspect, FOV);
		m_RotateDampeningFactor = 0.00001f;
	}

	MayaCamera::MayaCamera(float pitch, float yaw, const maths::Vector3& position, float FOV, float Near, float Far, float aspect)
		: Camera(pitch, yaw, position, FOV, Near, Far, aspect), m_Free(false)
	{
		Application::Instance()->GetWindow()->HideMouse(false);

		m_PanSpeed = 0.002f;
		m_RotationSpeed = 1.2f;
		m_ZoomSpeed = 0.6f;

		m_FocalPoint = maths::Vector3::Zero();
		maths::Vector3 distance = (m_Position - m_FocalPoint);
		m_Distance = distance.Length();// m_Position.Distance(m_FocalPoint);

		m_ProjMatrix = maths::Matrix4::Perspective(m_Near, m_Far, aspect, FOV);
		m_RotateDampeningFactor = 0.00001f;
	}

	MayaCamera::~MayaCamera()
	{
	}

	void MayaCamera::HandleMouse(float dt, float xpos, float ypos)
	{
		const maths::Vector2 delta = (maths::Vector2(xpos, ypos) - m_PreviousCurserPos);

		if (Input::GetInput().GetMouseHeld(LUMOS_MOUSE_MIDDLE))
			MousePan(delta);
		else if (Input::GetInput().GetMouseHeld(LUMOS_MOUSE_LEFT))
			MouseRotate(delta, dt);
		else if (Input::GetInput().GetMouseHeld(LUMOS_MOUSE_RIGHT))
			MouseZoom(delta.GetY(), dt);

		float yawSign = GetUpDirection().GetY() < 0 ? -1.0f : 1.0f;
		m_Yaw -= yawSign * m_RotateVelocity.GetX()  * dt;
		m_Pitch -= m_RotateVelocity.GetY()  * dt;

		m_Distance -= m_ZoomVelocity * dt;
		m_FocalPoint += GetForwardDirection() * m_ZoomVelocity * dt;

		m_ZoomVelocity = m_ZoomVelocity * pow(m_ZoomDampeningFactor, dt);
		m_RotateVelocity = m_RotateVelocity * pow(m_RotateDampeningFactor, dt);

		if (m_Yaw < 0)
			m_Yaw += 360.0f;

		if (m_Yaw > 360.0f)
			m_Yaw -= 360.0f;

		m_PreviousCurserPos = maths::Vector2(xpos, ypos);
		m_Position = CalculatePosition();
	}

	void MayaCamera::HandleKeyboard(float dt)
	{
		//	Camera::HandleKeyboard(dt);
	}

	void MayaCamera::MousePan(const maths::Vector2& delta)
	{
		m_FocalPoint -= -GetRightDirection() * delta.GetX() * m_PanSpeed * m_Distance;
		m_FocalPoint -= GetUpDirection() * delta.GetY() * m_PanSpeed * m_Distance;
	}

	void MayaCamera::MouseRotate(const maths::Vector2& delta, const float dt)
	{
		m_RotateVelocity = m_RotateVelocity + delta *  m_RotationSpeed;
	}

	void MayaCamera::MouseZoom(float delta, const float dt)
	{
		m_ZoomVelocity = m_ZoomVelocity + delta * m_ZoomSpeed;
	}
}
