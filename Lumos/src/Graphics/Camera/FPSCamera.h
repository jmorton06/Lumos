#pragma once
#include "lmpch.h"
#include "CameraController.h"

namespace Lumos
{

	class LUMOS_EXPORT FPSCameraController : CameraController
	{
	public:
		FPSCameraController(Camera* camera);
		virtual ~FPSCameraController() override;

		virtual void HandleMouse(float dt, float xpos, float ypos) override;
		virtual void HandleKeyboard(float dt) override;
	};

}

