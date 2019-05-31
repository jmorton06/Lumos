#pragma once
#include "LM.h"
#include "LumosComponent.h"

namespace lumos
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
		
		Camera* GetCamera() const { return m_Camera; }

    private:
		Camera* m_Camera;
	};
}
