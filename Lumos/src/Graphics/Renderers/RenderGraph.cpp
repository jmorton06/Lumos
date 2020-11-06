#include "Precompiled.h"
#include "RenderGraph.h"
#include "Graphics/Renderers/IRenderer.h"

namespace Lumos::Graphics
{
    
    RenderGraph::RenderGraph()
    {

    }

    RenderGraph::~RenderGraph()
    {
        
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
