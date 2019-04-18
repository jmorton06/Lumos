#pragma once

#include "LM.h"
#include "Light.h"

namespace Lumos
{

	class LUMOS_EXPORT LightSetup
	{
	public:
		LightSetup();
		~LightSetup();

		void Add(std::shared_ptr<Light>& light);
		void Remove(std::shared_ptr<Light>& light);

		void SetDirectionalLight(std::shared_ptr<Light>& light) { m_DirectionalLight = light; }
		const std::vector<std::shared_ptr<Light >>& GetLights() const { return m_Lights; }
		int GetNumLights() const { return static_cast<int>(m_Lights.size()); }
		void Clear();

		Light* GetDirectionalLight() const { return m_DirectionalLight.get(); }
		maths::Vector3 GetDirectionalLightPosition() const { return m_DirectionalLight ? m_DirectionalLight->GetPosition() : maths::Vector3(0.0f); }
		maths::Vector3 GetDirectionalLightDirection() const { return m_DirectionalLight ? m_DirectionalLight->GetDirection() : maths::Vector3(0.0f); }

		void OnImGUI();

	private:
		std::vector<std::shared_ptr<Light>> m_Lights;
		std::shared_ptr<Light> m_DirectionalLight;
	};
}
