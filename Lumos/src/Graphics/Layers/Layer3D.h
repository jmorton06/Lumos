#pragma once

#include "Core.h"
#include "Layer.h"
#include "Events/ApplicationEvent.h"

namespace lumos
{
	namespace graphics
	{
		class Renderer3D;
	}

    class LUMOS_EXPORT Layer3D : public Layer
	{
	public:
		Layer3D(graphics::Renderer3D* renderer, const std::string& name = "Layer3D");
		virtual ~Layer3D();

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnUpdate(TimeStep* dt,Scene* scene) override;
		virtual void OnEvent(Event& event) override;
		virtual void OnRender(Scene* scene) override;

    protected:
        Scene* m_Scene;
		graphics::Renderer3D* m_Renderer;
	private:
        bool OnwindowResizeEvent(WindowResizeEvent& e);
	};

}
