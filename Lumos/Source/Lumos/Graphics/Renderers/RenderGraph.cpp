#include "Precompiled.h"
#include "RenderGraph.h"
#include "Graphics/GBuffer.h"
#include "Graphics/Renderers/IRenderer.h"
#include "Graphics/Renderers/DebugRenderer.h"

#include "Events/ApplicationEvent.h"

namespace Lumos::Graphics
{
    RenderGraph::RenderGraph(uint32_t width, uint32_t height)
    {
        LUMOS_PROFILE_FUNCTION();
        SetScreenBufferSize(width, height);

        m_GBuffer = new GBuffer(width, height);
        Reset();
    }

    RenderGraph::~RenderGraph()
    {
        delete m_GBuffer;
        for(auto renderer : m_Renderers)
        {
            delete renderer;
        }

        DebugRenderer::Release();
    }

    void RenderGraph::OnResize(uint32_t width, uint32_t height)
    {
        LUMOS_PROFILE_FUNCTION();
        SetScreenBufferSize(width, height);
        m_GBuffer->UpdateTextureSize(width, height);
    }

    void RenderGraph::EnableDebugRenderer(bool enable)
    {
        if(enable)
            DebugRenderer::Init(m_ScreenBufferWidth, m_ScreenBufferHeight);
        else
            DebugRenderer::Release();
    }

    void RenderGraph::BeginScene(Scene* scene)
    {
        LUMOS_PROFILE_FUNCTION();
        DebugRenderer::Reset();

        for(auto renderer : m_Renderers)
        {
            renderer->BeginScene(scene, m_OverrideCamera, m_OverrideCameraTransform);
        }

        DebugRenderer::BeginScene(scene, m_OverrideCamera, m_OverrideCameraTransform);
    }

    void RenderGraph::SetRenderTarget(Graphics::Texture* texture, bool onlyIfTargetsScreen, bool rebuildFramebuffer)
    {
        LUMOS_PROFILE_FUNCTION();
        for(auto renderer : m_Renderers)
        {
            if(!onlyIfTargetsScreen || renderer->GetScreenRenderer())
                renderer->SetRenderTarget(texture, rebuildFramebuffer);
        }

        //DebugRenderer?
    }

    void RenderGraph::OnRender()
    {
        LUMOS_PROFILE_FUNCTION();
        for(auto renderer : m_Renderers)
        {
            renderer->RenderScene();
        }
    }

    void RenderGraph::OnUpdate(const TimeStep& timeStep, Scene* scene)
    {
    }

    bool RenderGraph::OnwindowResizeEvent(WindowResizeEvent& e)
    {
        LUMOS_PROFILE_FUNCTION();
        for(auto renderer : m_Renderers)
        {
            renderer->OnResize(e.GetWidth(), e.GetHeight());
        }

        DebugRenderer::OnResize(e.GetWidth(), e.GetHeight());

        return false;
    }

    void RenderGraph::OnEvent(Event& e)
    {
        LUMOS_PROFILE_FUNCTION();
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(RenderGraph::OnwindowResizeEvent));
    }

    void RenderGraph::OnImGui()
    {
        LUMOS_PROFILE_FUNCTION();
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
        LUMOS_PROFILE_FUNCTION();
        std::sort(m_Renderers.begin(), m_Renderers.end(), [](Graphics::IRenderer* a, Graphics::IRenderer* b)
            { return a->GetRenderPriority() > b->GetRenderPriority(); });
    }
}
