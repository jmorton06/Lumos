#pragma once

#include "Core.h"
#include "Events/Event.h"
#include "Utilities/TimeStep.h"

namespace lumos
{
	class Scene;

	class LUMOS_EXPORT Layer
	{
	public:
		Layer(const std::string& name = "Layer");
		virtual ~Layer();

		virtual void OnAttach() {}
		virtual void OnDetach() {}
		virtual void OnUpdate(TimeStep* dt, Scene* scene) {}
		virtual void OnEvent(Event& event) {}
		virtual void OnRender(Scene* scene) {}
        virtual void OnNewScene(Scene* scene) {}
		virtual void OnIMGUI() {}

		inline const std::string& GetName() const { return m_DebugName; }
	protected:
		std::string m_DebugName;
	};

}
