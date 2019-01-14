#include "LM.h"
#include "LightSetUp.h"

namespace Lumos
{

	LightSetup::LightSetup()
	{
		m_DirectionalLight = nullptr;
	}

	LightSetup::~LightSetup()
	{
		delete m_DirectionalLight;
	};

	void LightSetup::Add(std::shared_ptr<Light> light)
	{
		m_Lights.push_back(light);
	}

	void LightSetup::Remove(std::shared_ptr<Light> light)
	{
		for (uint i = 0; i < m_Lights.size(); i++)
		{
			if (m_Lights[i] == light)
			{
				m_Lights.erase(m_Lights.begin() + i);
				break;
			}
		}
	}

	void LightSetup::Clear()
	{
		m_Lights.clear();
		delete m_DirectionalLight;
		m_DirectionalLight = nullptr;
	}
}
