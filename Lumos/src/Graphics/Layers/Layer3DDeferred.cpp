#include "LM.h"
#include "Layer3DDeferred.h"
#include "Graphics/Renderers/Renderer3D.h"
#include "Graphics/API/IMGUIRenderer.h"
#include "Renderer/Scene.h"
#include "Graphics/API/Renderer.h"
#include "Graphics/API/Swapchain.h"
#include "Graphics/Renderers/DeferredRenderer.h"

namespace Lumos
{
    Layer3DDeferred::Layer3DDeferred(Scene* scene, Renderer3D* renderer, const std::string& debugName)
    {
        m_Scene = scene;
        m_Renderer = renderer;
        
        m_IMGUIRenderer = std::unique_ptr<graphics::api::IMGUIRenderer>(graphics::api::IMGUIRenderer::Create(scene->GetScreenWidth(),scene->GetScreenHeight()));
        
        if(m_IMGUIRenderer)
            m_IMGUIRenderer->Init();
    }
    
    Layer3DDeferred::Layer3DDeferred()
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
        ((DeferredRenderer*)m_Renderer)->RenderScene(scene->GetRenderList(), scene);
        
        {
            int commandBufferIndex = Renderer::GetRenderer()->GetSwapchain()->GetCurrentBufferId();
            ((DeferredRenderer*)m_Renderer)->Begin(commandBufferIndex);
            m_Renderer->Present();
            
            if(m_IMGUIRenderer && m_IMGUIRenderer->Implemented())
            {
                m_IMGUIRenderer->Render(((DeferredRenderer*)m_Renderer)->GetCommandBuffer(commandBufferIndex));
            }
            m_Renderer->End();
        }
        
        ((DeferredRenderer*)m_Renderer)->PresentToScreen();
    }
}
