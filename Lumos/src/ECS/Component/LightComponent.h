#pragma once
#include "LM.h"
#include "LumosComponent.h"
#include "Maths/Vector3.h"
#include "Graphics/Light.h"

namespace Lumos
{
	class LUMOS_EXPORT LightComponent : public LumosComponent
	{
	public:
        LightComponent();
		explicit LightComponent(const Ref<Graphics::Light>& light);
        ~LightComponent();

		void SetRadius(float radius);

		void Init();
		void Update();

		void OnIMGUI() override;

		Ref<Graphics::Light> GetLight() const { return m_Light; }

		nlohmann::json Serialise() override;;
		void Deserialise(nlohmann::json& data) override;;
        
    private:
        Ref<Graphics::Light> m_Light;
	};
}
