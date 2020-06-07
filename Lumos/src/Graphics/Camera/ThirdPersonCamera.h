#pragma once
#include "CameraController.h"

namespace Lumos
{

	class LUMOS_EXPORT ThirdPersonCameraController : public CameraController
	{
	public:
		ThirdPersonCameraController();
		virtual ~ThirdPersonCameraController() override;

		virtual void HandleMouse(Camera* camera, float dt, float xpos, float ypos) override;
		virtual void HandleKeyboard(Camera* camera, float dt) override;

	private:
		bool m_Free;
	};
}

