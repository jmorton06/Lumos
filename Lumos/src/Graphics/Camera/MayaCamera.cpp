#include "lmpch.h"
#include "MayaCamera.h"
#include "Core/OS/Input.h"
#include "Camera.h"

#include <imgui/imgui.h>

namespace Lumos
{

	MayaCameraController::MayaCameraController(Camera* camera)
		: CameraController(camera)
	{
		m_PanSpeed = 0.002f;
		m_RotationSpeed = 1.2f;
		m_ZoomSpeed = 0.6f;

		m_FocalPoint = Maths::Vector3::ZERO;
		Maths::Vector3 distance = (camera->GetPosition() - m_FocalPoint);
		m_Distance = distance.Length();

		m_RotateDampeningFactor = 0.00001f;
	}

	MayaCameraController::~MayaCameraController()
	{
	}

	void MayaCameraController::HandleMouse(float dt, float xpos, float ypos)
	{
		const Maths::Vector2 delta = (Maths::Vector2(xpos, ypos) - m_PreviousCurserPos);

		if (Input::GetInput()->GetMouseHeld(LUMOS_MOUSE_MIDDLE))
			MousePan(delta);
		else if (Input::GetInput()->GetMouseHeld(LUMOS_MOUSE_LEFT))
			MouseRotate(delta, dt);
		else if (Input::GetInput()->GetMouseHeld(LUMOS_MOUSE_RIGHT))
			MouseZoom(delta.y, dt);

		float yawSign = m_Camera->GetUpDirection().y < 0 ? -1.0f : 1.0f;
    
        float yaw = m_Camera->GetYaw();
        float pitch = m_Camera->GetPitch();
    
		yaw -= yawSign * m_RotateVelocity.x  * dt;
		pitch -= m_RotateVelocity.y  * dt;

		m_Distance -= m_ZoomVelocity * dt;
		m_FocalPoint += m_Camera->GetForwardDirection() * m_ZoomVelocity * dt;

		m_ZoomVelocity = m_ZoomVelocity * pow(m_ZoomDampeningFactor, dt);
		m_RotateVelocity = m_RotateVelocity * pow(m_RotateDampeningFactor, dt);

		if (yaw < 0)
			yaw += 360.0f;

		if (yaw > 360.0f)
			yaw -= 360.0f;

		m_PreviousCurserPos = Maths::Vector2(xpos, ypos);
    
        Maths::Vector3 pos = m_Camera->GetPosition();
		pos = m_Camera->CalculatePosition();
        m_Camera->SetPosition(pos);
	}

	void MayaCameraController::HandleKeyboard(float dt)
	{
		//	Camera::HandleKeyboard(dt);
	}

	void MayaCameraController::MousePan(const Maths::Vector2& delta)
	{
		m_FocalPoint -= -m_Camera->GetRightDirection() * delta.x * m_PanSpeed * m_Distance;
		m_FocalPoint -= m_Camera->GetUpDirection() * delta.y * m_PanSpeed * m_Distance;
	}

	void MayaCameraController::MouseRotate(const Maths::Vector2& delta, const float dt)
	{
		m_RotateVelocity = m_RotateVelocity + m_RotationSpeed * delta ;
	}

	void MayaCameraController::MouseZoom(float delta, const float dt)
	{
		m_ZoomVelocity = m_ZoomVelocity + delta * m_ZoomSpeed;
	}
}
