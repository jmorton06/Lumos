#include "Precompiled.h"
#include "RenderGraph.h"
#include "Graphics/GBuffer.h"
#include "Graphics/Renderers/IRenderer.h"

namespace Lumos::Graphics
{
	RenderGraph::RenderGraph(u32 width, u32 height)
	{
		SetScreenBufferSize(width, height);
		
		m_GBuffer = new GBuffer(width, height);
		Reset();
	}
	
	RenderGraph::~RenderGraph() { delete m_GBuffer; }
	
	void RenderGraph::OnResize(u32 width, u32 height)
	{
		SetScreenBufferSize(width, height);
		m_GBuffer->UpdateTextureSize(width, height);
	}
	
	void RenderGraph::Reset()
	{
		m_ReflectSkyBox = false;
		m_UseShadowMap = false;
		m_NumShadowMaps = 4;
	}

    void RenderGraph::AddRenderer(Graphics::IRenderer* renderer)
    {
        m_Renderers.push_back(renderer);
        SortRenderers();
    }

    void RenderGraph::SortRenderers()
    {
		std::sort(m_Renderers.begin(), m_Renderers.end(), [](Graphics::IRenderer* a, Graphics::IRenderer* b) { return a->GetRenderPriority() > b->GetRenderPriority(); });
    }
}
