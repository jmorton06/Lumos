#pragma once
#include "LM.h"
#include "LumosComponent.h"
#include "Maths/Vector3.h"


namespace Lumos
{
	namespace Graphics
	{
		struct Light;
	}

	class LUMOS_EXPORT LightComponent : public LumosComponent
	{
	public:
        LightComponent();
		explicit LightComponent(std::shared_ptr<Graphics::Light>& light);
        ~LightComponent();

		void SetRadius(float radius);

		void OnUpdateComponent(float dt) override;
		void Init() override;

		void OnIMGUI() override;

		std::shared_ptr<Graphics::Light> GetLight() const { return m_Light; }
		nlohmann::json Serialise() override { return nullptr; };
		void Deserialise(nlohmann::json& data) override {};
        
    private:
        std::shared_ptr<Graphics::Light> m_Light;
	};
}
