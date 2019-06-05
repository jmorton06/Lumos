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

		static ComponentType GetStaticType()
		{
            static ComponentType type(ComponentType::Camera);
			return type;
		}

		inline virtual ComponentType GetType() const override { return GetStaticType(); }
		void OnUpdateComponent(float dt) override;
        void OnIMGUI() override;
		void SetAsMainCamera();
		
		Camera* GetCamera() const { return m_Camera; }

    private:
		Camera* m_Camera;
	};
}
