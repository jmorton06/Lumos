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
	
	RenderGraph::~RenderGraph()
    {
        delete m_GBuffer;
        for(auto renderer: m_Renderers)
        {
            delete renderer;
        }
    }
	
	void RenderGraph::OnResize(u32 width, u32 height)
	{
		SetScreenBufferSize(width, height);
		m_GBuffer->UpdateTextureSize(width, height);
	}

    void RenderGraph::BeginScene(Scene* scene)
    {
        for(auto renderer: m_Renderers)
        {
            renderer->BeginScene(scene, m_OverrideCamera , m_OverrideCameraTransform);
        }
    }

    void RenderGraph::SetRenderTarget(Graphics::Texture* texture, bool onlyIfTargetsScreen, bool rebuildFramebuffer)
    {
        for(auto renderer: m_Renderers)
        {
            if(!onlyIfTargetsScreen || renderer->GetScreenRenderer())
                renderer->SetRenderTarget(texture, rebuildFramebuffer);
        }
    }

    void RenderGraph::OnRender(Scene * scene)
    {
        for(auto renderer : m_Renderers)
        {
            renderer->BeginScene(scene, m_OverrideCamera, m_OverrideCameraTransform );
            renderer->RenderScene(scene);
        }
    }

    void RenderGraph::OnUpdate(const TimeStep& timeStep, Scene* scene)
    {
    }

    bool RenderGraph::OnwindowResizeEvent(WindowResizeEvent& e)
    {
        for(auto renderer : m_Renderers)
        {
            renderer->OnResize(e.GetWidth(), e.GetHeight());
        }
        return false;
    }

    void RenderGraph::OnEvent(Event& e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(RenderGraph::OnwindowResizeEvent));
    }

    void RenderGraph::OnImGui()
    {
        for(auto renderer : m_Renderers)
        {
            renderer->OnImGui();
        }
    }

    void RenderGraph::OnNewScene(Scene* scene)
    {
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
        //SortRenderers();
    }

    void RenderGraph::AddRenderer(Graphics::IRenderer* renderer, int renderPriority)
    {
        renderer->SetRenderPriority(renderPriority);
        m_Renderers.push_back(renderer);
        //SortRenderers();
    }

    void RenderGraph::SortRenderers()
    {
		std::sort(m_Renderers.begin(), m_Renderers.end(), [](Graphics::IRenderer* a, Graphics::IRenderer* b) { return a->GetRenderPriority() > b->GetRenderPriority(); });
    }
}
