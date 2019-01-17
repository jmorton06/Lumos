#include "LM.h"
#include "Layer3D.h"
#include "Graphics/Renderers/Renderer3D.h"

namespace Lumos
{
	Layer3D::Layer3D(Scene* scene, Renderer3D* renderer, const std::string& debugName)
		: m_Scene(scene), m_Renderer(renderer), Layer(debugName)
	{
	}

	Layer3D::Layer3D()
	{
	}

	void Layer3D::OnAttach()
	{
	}

	void Layer3D::OnDetach()
	{
	}

	void Layer3D::OnUpdate(TimeStep* dt)
	{
	}

	void Layer3D::OnEvent(Event& event)
	{
        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(Layer3D::OnwindowResizeEvent));
	}

	void Layer3D::OnRender(Scene* scene)
	{
	}
    
    bool Layer3D::OnwindowResizeEvent(WindowResizeEvent & e)
    {
        m_Renderer->OnResize(e.GetWidth(), e.GetHeight());
        
        return false;
    }
}
