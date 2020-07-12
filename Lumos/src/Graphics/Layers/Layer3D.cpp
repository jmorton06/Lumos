#include "lmpch.h"
#include "Layer3D.h"
#include "Graphics/Renderers/Renderer3D.h"
#include "App/Scene.h"

namespace Lumos
{
	Layer3D::Layer3D(Graphics::Renderer3D* renderer, const std::string& debugName)
		: m_Renderer(renderer)
		, Layer(debugName)
		, m_Scene(nullptr)
	{
	}

	Layer3D::~Layer3D()
	{
		delete m_Renderer;
	}

	void Layer3D::OnAttach()
	{
	}

	void Layer3D::OnDetach()
	{
	}

	void Layer3D::OnUpdate(const TimeStep& dt, Scene* scene)
	{
	}

	void Layer3D::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(Layer3D::OnwindowResizeEvent));
	}

	void Layer3D::OnRender(Scene* scene)
	{
		m_Renderer->BeginScene(scene, m_OverrideCamera);
		m_Renderer->RenderScene(scene);
	}

	void Layer3D::OnImGui()
	{
		m_Renderer->OnImGui();
	}

	bool Layer3D::OnwindowResizeEvent(WindowResizeEvent& e)
	{
		m_Renderer->OnResize(e.GetWidth(), e.GetHeight());
		return false;
	}

	void Layer3D::SetRenderTarget(Graphics::Texture* texture, bool onlyIfTargetsScreen, bool rebuildFramebuffer)
	{
		if(!onlyIfTargetsScreen || m_ScreenLayer)
			m_Renderer->SetRenderTarget(texture, rebuildFramebuffer);
	}
}
