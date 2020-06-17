#include "lmpch.h"
#include "FPSCamera.h"
#include "App/Application.h"
#include "Core/OS/Input.h"
#include "Core/OS/Window.h"
#include "Camera.h"

#include <imgui/imgui.h>

namespace Lumos
{

	FPSCameraController::FPSCameraController()
	{
	}

	FPSCameraController::~FPSCameraController() = default;

	void FPSCameraController::HandleMouse(Camera* camera, float dt, float xpos, float ypos)
	{
		if (Input::GetInput()->GetWindowFocus())
		{
			{
				Maths::Vector2 windowCentre = Maths::Vector2();
				xpos -= windowCentre.x;
				ypos -= windowCentre.y;

                float pitch = camera->GetPitch();
                float yaw = camera->GetYaw();
            
				pitch -= (ypos)* m_MouseSensitivity;
				yaw   -= (xpos)* m_MouseSensitivity;

				Application::Get().GetWindow()->SetMousePosition(windowCentre);

				if (yaw < 0)
					yaw += 360.0f;

				if (yaw > 360.0f)
					yaw -= 360.0f;
            
                camera->SetYaw(yaw);
                camera->SetPitch(pitch);
			}

			m_PreviousCurserPos = Maths::Vector2(xpos, ypos);

			UpdateScroll(camera, Input::GetInput()->GetScrollOffset(), dt);
		}
	}

	void FPSCameraController::HandleKeyboard(Camera* camera, float dt)
	{
        float pitch = camera->GetPitch();
        float yaw = camera->GetYaw();
    
		const Maths::Quaternion orientation = Maths::Quaternion::EulerAnglesToQuaternion(pitch, yaw, 1.0f);
		Maths::Vector3 up = Maths::Vector3(0, 1, 0), right = Maths::Vector3(1, 0, 0), forward = Maths::Vector3(0, 0, -1);
		up = orientation * up;
		right = orientation * right;
		forward = orientation * forward;

		m_CameraSpeed = 1000.0f * dt;

		if (Input::GetInput()->GetKeyHeld(LUMOS_KEY_W))
		{
			m_Velocity += forward * m_CameraSpeed;
		}

		if (Input::GetInput()->GetKeyHeld(LUMOS_KEY_S))
		{
			m_Velocity -= forward * m_CameraSpeed;
		}

		if (Input::GetInput()->GetKeyHeld(LUMOS_KEY_A))
		{
			m_Velocity -= right * m_CameraSpeed;
		}

		if (Input::GetInput()->GetKeyHeld(LUMOS_KEY_D))
		{
			m_Velocity += right * m_CameraSpeed;
		}

		if (Input::GetInput()->GetKeyHeld(LUMOS_KEY_SPACE))
		{
			m_Velocity -= up * m_CameraSpeed;
		}

		if (Input::GetInput()->GetKeyHeld(LUMOS_KEY_LEFT_SHIFT))
		{
			m_Velocity += up * m_CameraSpeed;
		}

        Maths::Vector3 pos = camera->GetPosition();
		pos += m_Velocity * dt;
		m_Velocity = m_Velocity * pow(m_DampeningFactor, dt);
    
        camera->SetPosition(pos);

	}
}
