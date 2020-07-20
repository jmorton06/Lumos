#pragma once
#include "lmpch.h"
#include "CameraController.h"

namespace Lumos
{

	class LUMOS_EXPORT FPSCameraController : public CameraController
	{
	public:
		FPSCameraController();
		virtual ~FPSCameraController() override;

		virtual void HandleMouse(Camera* camera, float dt, float xpos, float ypos) override;
		virtual void HandleKeyboard(Camera* camera, float dt) override;
        void UpdateScroll(Camera* camera, float offset, float dt) override { };

	};

}

