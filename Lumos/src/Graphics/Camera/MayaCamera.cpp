#include "lmpch.h"
#include "MayaCamera.h"
#include "Core/OS/Input.h"
#include "Camera.h"

#include <imgui/imgui.h>

namespace Lumos
{

	MayaCameraController::MayaCameraController()
	{
		m_PanSpeed = 0.002f;
		m_RotationSpeed = 1.2f;
		m_ZoomSpeed = 0.6f;

		m_FocalPoint = Maths::Vector3::ZERO;
		Maths::Vector3 distance = Maths::Vector3(1.0f,0.0f,0.0f);
		m_Distance = distance.Length();

		m_RotateDampeningFactor = 0.00001f;
	}

	MayaCameraController::~MayaCameraController()
	{
	}

	void MayaCameraController::HandleMouse(Camera* camera, float dt, float xpos, float ypos)
	{
		const Maths::Vector2 delta = (Maths::Vector2(xpos, ypos) - m_PreviousCurserPos);

		if (Input::GetInput()->GetMouseHeld(LUMOS_MOUSE_MIDDLE))
			MousePan(camera, delta);
		else if (Input::GetInput()->GetMouseHeld(LUMOS_MOUSE_LEFT))
			MouseRotate(delta, dt);
		else if (Input::GetInput()->GetMouseHeld(LUMOS_MOUSE_RIGHT))
			MouseZoom(delta.y, dt);

		float yawSign = camera->GetUpDirection().y < 0 ? -1.0f : 1.0f;
    
        float yaw = camera->GetYaw();
        float pitch = camera->GetPitch();
    
		yaw += yawSign * m_RotateVelocity.x  * dt;
		pitch += m_RotateVelocity.y  * dt;

		m_Distance -= m_ZoomVelocity * dt;
    
        if (m_Distance < 1.0f)
        {
            m_FocalPoint += camera->GetForwardDirection();
            m_Distance = 1.0f;
        }
    
		m_FocalPoint += camera->GetForwardDirection() * m_ZoomVelocity * dt;

		m_ZoomVelocity = m_ZoomVelocity * pow(m_ZoomDampeningFactor, dt);
		m_RotateVelocity = m_RotateVelocity * pow(m_RotateDampeningFactor, dt);

		if (yaw < 0)
			yaw += 360.0f;

		if (yaw > 360.0f)
			yaw -= 360.0f;

		m_PreviousCurserPos = Maths::Vector2(xpos, ypos);
		Maths::Vector3 pos = CalculatePosition(camera);
        camera->SetPosition(pos);
        camera->SetYaw(yaw);
        camera->SetPitch(pitch);
	}

	void MayaCameraController::HandleKeyboard(Camera* camera, float dt)
	{
		//	Camera::HandleKeyboard(dt);
	}
    
    Maths::Vector3 MayaCameraController::CalculatePosition(Camera* camera) const
    {
        return m_FocalPoint - camera->GetForwardDirection() * m_Distance;
    }

	void MayaCameraController::MousePan(Camera* camera, const Maths::Vector2& delta)
	{
		m_FocalPoint += -camera->GetRightDirection() * delta.x * m_PanSpeed * m_Distance;
		m_FocalPoint += camera->GetUpDirection() * delta.y * m_PanSpeed * m_Distance;
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
