#include "LM.h"
#include "LayerStack.h"

namespace Lumos
{

	LayerStack::LayerStack()
	{
	}

	LayerStack::~LayerStack()
	{
		for (Layer* layer : m_Layers)
			delete layer;
	}

	void LayerStack::PushLayer(Layer* layer)
	{
		m_Layers.push_back(layer);
	}

	void LayerStack::PushOverlay(Layer* overlay)
	{
		m_Overlays.push_back(overlay);
	}

	void LayerStack::PopLayer(Layer* layer)
	{
		auto it = std::find(m_Layers.begin(), m_Layers.end(), layer);
		if (it != m_Layers.end())
		{
			m_Layers.erase(it);
		}
	}

	void LayerStack::PopOverlay(Layer* overlay)
	{
		auto it = std::find(m_Overlays.begin(), m_Overlays.end(), overlay);
		if (it != m_Overlays.end())
			m_Overlays.erase(it);
	}

	void LayerStack::OnRender(Scene * scene)
	{
		for (uint i = 0; i < m_Layers.size(); i++)
		{
			Layer* layer = m_Layers[i];
			layer->OnRender(scene);
		}

		for (uint i = 0; i < m_Overlays.size(); i++)
		{
			Layer* layer = m_Overlays[i];
			layer->OnRender(scene);
		}
	}

	void LayerStack::OnUpdate(TimeStep* timeStep)
	{
		for (uint i = 0; i < m_Overlays.size(); i++)
		{
			Layer* layer = m_Overlays[i];
			layer->OnUpdate(timeStep);
		}

		for (uint i = 0; i < m_Layers.size(); i++)
		{
			Layer* layer = m_Layers[i];
			layer->OnUpdate(timeStep);
		}
	}

	void LayerStack::OnEvent(Event& e)
	{
		for (uint i = 0; i < m_Overlays.size(); i++)
		{
			Layer* layer = m_Overlays[i];
			layer->OnEvent(e);

			if (e.Handled())
				break;
		}

		for (uint i = 0; i < m_Layers.size(); i++)
		{
			Layer* layer = m_Layers[i];
			layer->OnEvent(e);

			if (e.Handled())
				break;
		}
	}
    
    void LayerStack::Clear()
    {
        for (Layer* layer : m_Layers)
            delete layer;
        
        m_Layers.clear();

		for (Layer* overLay : m_Overlays)
			delete overLay;

		m_Overlays.clear();
    }
}
