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
        void Clear();

		std::deque<Layer*>::iterator begin() { return m_Layers.begin(); }
		std::deque<Layer*>::iterator end() { return m_Layers.end(); }
        
        uint GetCount() const { return (uint)m_Layers.size(); }
	private:
		std::deque<Layer*> m_Layers;
	};

}
