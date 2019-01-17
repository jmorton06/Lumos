#pragma once

#include "Core.h"
#include "Layer.h"
#include "Events/ApplicationEvent.h"

namespace Lumos
{
    class Renderer3D;
    
    class LUMOS_EXPORT Layer3D : public Layer
	{
	public:
		Layer3D(Scene* scene, Renderer3D* renderer, const std::string& name = "Layer3D");
		Layer3D();

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnUpdate(TimeStep* dt) override;
		virtual void OnEvent(Event& event) override;
		virtual void OnRender(Scene* scene) override;
    
    protected:
        Scene* m_Scene;
        Renderer3D* m_Renderer;
	private:
        bool OnwindowResizeEvent(WindowResizeEvent& e);
	};

}
