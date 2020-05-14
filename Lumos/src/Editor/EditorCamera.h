#pragma once
#include "Graphics/Camera/CameraController.h"

namespace Lumos
{
	class LUMOS_EXPORT EditorCameraController : public CameraController
	{
	public:
		EditorCameraController(Camera* camera);
		~EditorCameraController();

		virtual void HandleMouse(float dt, float xpos, float ypos) override;
		virtual void HandleKeyboard(float dt) override;
    
        void UpdateScroll(float offset, float dt) override;

	private:
    
    };
}
