#include "LM.h"
#include "Layer3DDeferred.h"
#include "Graphics/Renderers/Renderer3D.h"
#include "Graphics/API/IMGUIRenderer.h"
#include "App/Scene.h"
#include "Graphics/API/Renderer.h"
#include "Graphics/API/Swapchain.h"
#include "Graphics/Renderers/DeferredRenderer.h"

#include "App/Application.h"

namespace Lumos
{
    Layer3DDeferred::Layer3DDeferred(uint width, uint height, const std::string& debugName)
        : Layer3D(new DeferredRenderer(width, height), debugName)
    {

    }

    Layer3DDeferred::~Layer3DDeferred()
    {
    }

    void Layer3DDeferred::OnAttach()
    {
    }

    void Layer3DDeferred::OnDetach()
    {
    }

    void Layer3DDeferred::OnUpdate(TimeStep* dt)
    {
    }

    void Layer3DDeferred::OnEvent(Event& event)
    {
        Layer3D::OnEvent(event);
    }

    void Layer3DDeferred::OnRender(Scene* scene)
    {
        m_Renderer->RenderScene(scene->GetRenderList(), scene);
    }
}
