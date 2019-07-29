#pragma once
#include "LM.h"
#include "LumosComponent.h"

namespace Lumos
{
	class Camera;

	class LUMOS_EXPORT CameraComponent : public LumosComponent
	{
	public:
        CameraComponent();
		explicit CameraComponent(Camera* camera);

		void OnUpdateComponent(float dt) override;
        void OnIMGUI() override;
		void SetAsMainCamera();
		
		Camera* GetCamera() const { return m_Camera; }
		nlohmann::json Serialise() override { return nullptr; };
		void Deserialise(nlohmann::json& data) override {};

    private:
		Camera* m_Camera;
	};
}
