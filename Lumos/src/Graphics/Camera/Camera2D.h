#pragma once
#include "lmpch.h"
#include "CameraController.h"

namespace Lumos
{

	class LUMOS_EXPORT CameraController2D : public CameraController
	{
	public:
		CameraController2D(Camera* camera);
		virtual ~CameraController2D() override;

		virtual void HandleMouse(float dt, float xpos, float ypos) override;
		virtual void HandleKeyboard(float dt) override;
        
        void UpdateScroll(float offset, float dt) override;
	};
}

