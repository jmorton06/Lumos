#include "Precompiled.h"
#include "Layer2D.h"
#include "Graphics/Renderers/Renderer2D.h"
#include "Scene/Scene.h"

namespace Lumos
{
	Layer2D::Layer2D(Graphics::Renderer2D* renderer, const std::string& debugName)
		: Layer(debugName)
		, m_Scene(nullptr)
		, m_Renderer(renderer)
	{
	}

	Layer2D::~Layer2D()
	{
	}

	void Layer2D::OnAttach()
	{
	}

	void Layer2D::OnDetach()
	{
	}

	void Layer2D::OnUpdate(const TimeStep& dt, Scene* scene)
	{
		m_Renderer->BeginScene(scene, m_OverrideCamera, m_OverrideCameraTransform);
	}

	void Layer2D::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(Layer2D::OnwindowResizeEvent));
	}

	void Layer2D::OnRender(Scene* scene)
	{
		m_Renderer->RenderScene(scene);
	}

	bool Layer2D::OnwindowResizeEvent(WindowResizeEvent& e)
	{
		m_Renderer->OnResize(e.GetWidth(), e.GetHeight());
		return false;
	}

	void Layer2D::SetRenderTarget(Graphics::Texture* texture, bool onlyIfTargetsScreen, bool rebuildFramebuffer)
	{
		if(!onlyIfTargetsScreen || m_ScreenLayer)
			m_Renderer->SetRenderTarget(texture, rebuildFramebuffer);
	}
}
