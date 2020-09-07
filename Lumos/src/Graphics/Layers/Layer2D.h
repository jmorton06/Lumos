#pragma once

#include "Core/Core.h"
#include "Layer.h"
#include "Events/ApplicationEvent.h"

namespace Lumos
{
	namespace Graphics
	{
		class Renderer2D;
	}

    class LUMOS_EXPORT Layer2D : public Layer
	{
	public:
		Layer2D(Graphics::Renderer2D* renderer, const std::string& name = "Layer2D");
		virtual ~Layer2D();

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnUpdate(const TimeStep& dt, Scene* scene) override;
		virtual void OnEvent(Event& event) override;
		virtual void OnRender(Scene* scene) override;
        virtual void SetRenderTarget(Graphics::Texture* texture, bool onlyIfTargetsScreen, bool rebuildFramebuffer) override;

    protected:
        Scene* m_Scene;
        UniqueRef<Graphics::Renderer2D> m_Renderer;
	private:
        bool OnwindowResizeEvent(WindowResizeEvent& e);
	};

}
