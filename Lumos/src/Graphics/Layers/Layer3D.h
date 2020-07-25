#pragma once

#include "Core/Core.h"
#include "Layer.h"
#include "Events/ApplicationEvent.h"

namespace Lumos
{
	namespace Graphics
	{
		class Renderer3D;
	}

    class LUMOS_EXPORT Layer3D : public Layer
	{
	public:
		Layer3D(Graphics::Renderer3D* renderer, const std::string& name = "Layer3D");
		virtual ~Layer3D();

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnUpdate(const TimeStep& dt,Scene* scene) override;
		virtual void OnEvent(Event& event) override;
		virtual void OnRender(Scene* scene) override;
		virtual void OnImGui() override;
        virtual void SetRenderTarget(Graphics::Texture* texture, bool onlyIfTargetsScreen, bool rebuildFramebuffer) override;

    protected:
        Scene* m_Scene;
		Graphics::Renderer3D* m_Renderer;
	private:
        bool OnwindowResizeEvent(WindowResizeEvent& e);
	};

}
