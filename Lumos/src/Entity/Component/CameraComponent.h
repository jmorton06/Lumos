#pragma once
#include "LM.h"
#include "LumosComponent.h"

namespace Lumos
{
	class Camera;

	class LUMOS_EXPORT CameraComponent : public LumosComponent
	{
	public:
		explicit CameraComponent(Camera* camera);

		void OnUpdateComponent(float dt) override;
        void OnIMGUI() override;
		void SetAsMainCamera();
		
		Camera* GetCamera() const { return m_Camera; }

    private:
		Camera* m_Camera;
	};
}
