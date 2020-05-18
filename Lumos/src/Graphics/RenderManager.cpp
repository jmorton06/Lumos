#include "lmpch.h"
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
			m_GBuffer->UpdateTextureSize(width, height);
		}

		void RenderManager::Reset()
		{
			m_ReflectSkyBox = false;
			m_UseShadowMap = false;
			m_NumShadowMaps = 4;
			m_ShadowTexture = nullptr;
			m_ShadowRenderer = nullptr;
		}
	}
}
