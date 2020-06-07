#pragma once
#include "lmpch.h"
#include "CameraController.h"

namespace Lumos
{

	class LUMOS_EXPORT CameraController2D : public CameraController
	{
	public:
		CameraController2D();
		virtual ~CameraController2D() override;

		virtual void HandleMouse(Camera* camera, float dt, float xpos, float ypos) override;
		virtual void HandleKeyboard(Camera* camera, float dt) override;
        
        void UpdateScroll(Camera* camera, float offset, float dt) override;
	};
}

