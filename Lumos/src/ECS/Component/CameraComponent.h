#pragma once
#include "lmpch.h"

namespace Lumos
{
	class Camera;

	class LUMOS_EXPORT CameraComponent
	{
	public:
        CameraComponent();
		explicit CameraComponent(Camera* camera);

        void OnImGui();
		void SetAsMainCamera();
		
		Camera* GetCamera() const { return m_Camera; }

    private:
		Camera* m_Camera;
	};
}
