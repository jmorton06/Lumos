#pragma once
#include "LM.h"
#include "Camera.h"

namespace Lumos
{

	class LUMOS_EXPORT FPSCamera : public Camera
	{
	public:
		FPSCamera(float FOV, float Near, float Far, float aspect);
		FPSCamera(float pitch, float yaw, const Maths::Vector3& position, float FOV, float Near, float Far, float aspect);
		virtual ~FPSCamera() override;

		virtual void HandleMouse(float dt, float xpos, float ypos) override;
		virtual void HandleKeyboard(float dt) override;

		void OnImGui() override;
	};

}

