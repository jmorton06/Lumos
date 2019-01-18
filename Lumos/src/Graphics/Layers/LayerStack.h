#pragma once

#include "Core.h"
#include "Layer.h"

#include <deque>

namespace Lumos
{

	class LUMOS_EXPORT LayerStack
	{
	public:
		LayerStack();
		~LayerStack();

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* overlay);
		void PopLayer(Layer* layer);
		void PopOverlay(Layer* overlay);

		std::deque<Layer*>::iterator begin() { return m_Layers.begin(); }
		std::deque<Layer*>::iterator end() { return m_Layers.end(); }
	private:
		std::deque<Layer*> m_Layers;
	};

}