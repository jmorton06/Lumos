#include "LM.h"
#include "RenderManager.h"

namespace Lumos
{
    void RenderManager::Reset()
    {
        m_ReflectSkyBox = false;
        m_UseShadowMap  = false;
        m_NumShadowMaps = 4;
        m_SkyBoxTexture = nullptr;
        m_ShadowTexture = nullptr;
		m_ShadowRenderer = nullptr;
    }
}
