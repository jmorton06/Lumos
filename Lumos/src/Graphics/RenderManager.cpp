#include "LM.h"
#include "RenderManager.h"
#include "GBuffer.h"

namespace Lumos
{
	namespace Graphics
	{
		RenderManager::RenderManager(u32 width, u32 height)
		{
			SetScreenBufferSize(width, height);

			m_GBuffer = lmnew GBuffer(width, height);
			Reset();
		}
		RenderManager::~RenderManager() { delete m_GBuffer; }

		void RenderManager::OnResize(u32 width, u32 height)
		{
			SetScreenBufferSize(width, height);

			delete m_GBuffer;
			m_GBuffer = lmnew GBuffer(width, height);
		}

		void RenderManager::Reset()
		{
			m_ReflectSkyBox = false;
			m_UseShadowMap = false;
			m_NumShadowMaps = 4;
			m_SkyBoxTexture = nullptr;
			m_ShadowTexture = nullptr;
			m_ShadowRenderer = nullptr;
			m_SkyBoxTexture = nullptr;
		}
	}
}
