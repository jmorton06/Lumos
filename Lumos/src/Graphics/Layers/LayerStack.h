#pragma once

#include "Core.h"
#include "Layer.h"

#include <deque>

namespace Lumos
{
	struct TimeStep;
	class Scene;
	class Event;

	class LUMOS_EXPORT LayerStack
	{
	public:
		LayerStack();
		~LayerStack();

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* overlay);
		void PopLayer(Layer* layer);
		void PopOverlay(Layer* overlay);
		void Clear();

		void OnRender(Scene* scene);
		void OnUpdate(TimeStep* timeStep);
		void OnEvent(Event& e);

		uint GetCount() const { return (uint)m_Layers.size() + (uint)m_Overlays.size(); }

	private:
		std::vector<Layer*> m_Layers;
		std::vector<Layer*> m_Overlays;
	};
}
