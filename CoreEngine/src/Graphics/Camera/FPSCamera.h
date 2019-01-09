#pragma once
#include "JM.h"
#include "Camera.h"

namespace jm
{

	class JM_EXPORT FPSCamera : public Camera
	{
	public:
		FPSCamera(float FOV, float Near, float Far, float aspect);
		FPSCamera(float pitch, float yaw, const maths::Vector3& position, float FOV, float Near, float Far, float aspect);
		virtual ~FPSCamera() override;

		virtual void HandleMouse(float dt, float xpos, float ypos) override;
		virtual void HandleKeyboard(float dt) override;
	};

}

