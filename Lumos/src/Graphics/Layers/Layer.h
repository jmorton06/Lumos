#pragma once

#include "Core/Core.h"
#include "Events/Event.h"
#include "Utilities/TimeStep.h"

namespace Lumos
{
	class Scene;
    
    namespace Graphics
    {
        class Texture;
    }

	class LUMOS_EXPORT Layer
	{
	public:
		Layer(const std::string& name = "Layer");
		virtual ~Layer();

		virtual void OnAttach() {}
		virtual void OnDetach() {}
		virtual void OnUpdate(const TimeStep& dt, Scene* scene) {}
		virtual void OnEvent(Event& event) {}
		virtual void OnRender(Scene* scene) {}
        virtual void OnNewScene(Scene* scene) {}
		virtual void OnImGui() {}

		//onlyIfTargetsScreen : so editor can override layers that render to screen
        virtual void SetRenderTarget(Graphics::Texture* texture, bool onlyIfTargetsScreen = false, bool rebuildFramebuffer = true) {};

		_FORCE_INLINE_ const std::string& GetName() const { return m_DebugName; }
	protected:
		std::string m_DebugName;
		bool m_ScreenLayer = true;
	};

}
