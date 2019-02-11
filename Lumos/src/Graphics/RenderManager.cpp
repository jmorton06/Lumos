#include "LM.h"
#include "RenderManager.h"
#include "GBuffer.h"

namespace Lumos
{
    RenderManager::RenderManager(uint width, uint height)
    {
        SetScreenBufferSize(width,height);
        
        m_GBuffer = new GBuffer(width, height);
        Reset();
    }
	RenderManager::~RenderManager() { delete m_GBuffer; }
    
    void RenderManager::OnResize(uint width, uint height)
    {
        SetScreenBufferSize(width,height);

        delete m_GBuffer;
        m_GBuffer = new GBuffer(width, height);
    }

    void RenderManager::Reset()
    {
        m_ReflectSkyBox = false;
        m_UseShadowMap  = false;
        m_NumShadowMaps = 4;
        m_SkyBoxTexture = nullptr;
        m_ShadowTexture = nullptr;
		m_ShadowRenderer = nullptr;
        m_SkyBoxTexture = nullptr;
    }
}
