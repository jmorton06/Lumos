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
		void OnUpdate(TimeStep* timeStep, Scene* scene);
		void OnEvent(Event& e);
		void OnIMGUI();

		uint GetCount() const { return (uint)m_Layers.size(); }
        
        std::vector<Layer*>::iterator begin() { return m_Layers.begin(); }
        std::vector<Layer*>::iterator end() { return m_Layers.end(); }

	private:
        std::vector<Layer*> m_Layers;
		size_t m_LayerCount = 0;
	};
}
