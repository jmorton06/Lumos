#pragma once
#include "lmpch.h"
#include "Maths/Vector3.h"
#include "Graphics/Light.h"

#include <jsonhpp/json.hpp>

namespace Lumos
{
	class LUMOS_EXPORT LightComponent
	{
	public:
        LightComponent();
		explicit LightComponent(const Ref<Graphics::Light>& light);
        ~LightComponent();

		void SetRadius(float radius);

		void Init();
		void Update();

		void OnImGui();

		Ref<Graphics::Light> GetLight() const { return m_Light; }

		nlohmann::json Serialise();
		void Deserialise(nlohmann::json& data);
        
    private:
        Ref<Graphics::Light> m_Light;
	};
}
