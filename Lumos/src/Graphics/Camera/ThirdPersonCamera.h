#pragma once
#include "CameraController.h"

namespace Lumos
{

	class LUMOS_EXPORT ThirdPersonCameraController : public CameraController
	{
	public:
		ThirdPersonCameraController(Camera* camera);
		virtual ~ThirdPersonCameraController() override;

		virtual void HandleMouse(float dt, float xpos, float ypos) override;
		virtual void HandleKeyboard(float dt) override;

	private:
		bool m_Free;
	};
}

