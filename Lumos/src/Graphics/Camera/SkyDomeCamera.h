#pragma once
#include "JM.h"
#include "Camera.h"

namespace jm
{

	class JM_EXPORT SkyDomeCamera : public Camera
	{
	public:
		SkyDomeCamera(float Near, float Far);
		virtual ~SkyDomeCamera() override;

		virtual void HandleMouse(float dt, float xpos, float ypos) override {};
		virtual void HandleKeyboard(float dt) override {};

		void SwitchView(int id);
	};
}

