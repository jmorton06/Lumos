#include "LM.h"
#include "LayerStack.h"

#include "System/Profiler.h"

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
		m_Layers.emplace(m_Layers.begin() + m_LayerCount, layer);
		m_LayerCount++;

	}

	void LayerStack::PushOverlay(Layer* overlay)
	{
		m_Layers.emplace_back(overlay);
	}

	void LayerStack::PopLayer(Layer* layer)
	{
		auto it = std::find(m_Layers.begin(), m_Layers.begin() + m_LayerCount, layer);
		if (it != m_Layers.end())
		{
            delete *it;
			m_Layers.erase(it);
			m_LayerCount--;
		}
	}

	void LayerStack::PopOverlay(Layer* overlay)
	{
		auto it = std::find(m_Layers.begin() + m_LayerCount, m_Layers.end(), overlay);
		if (it != m_Layers.end())
        {
            delete *it;
            m_Layers.erase(it);
        }
	}

	void LayerStack::OnRender(Scene * scene)
	{
		for (uint i = 0; i < m_Layers.size(); i++)
		{
			Layer* layer = m_Layers[i];
            
			LUMOS_PROFILE(system::Profiler::OnBeginRange("Layer Render : " + layer->GetName(), true, "Render"));
			layer->OnRender(scene);
			LUMOS_PROFILE(system::Profiler::OnEndRange("Layer Render : " + layer->GetName(), true, "Render"));
		}
	}

	void LayerStack::OnUpdate(TimeStep* timeStep, Scene* scene)
	{
		for (uint i = 0; i < m_Layers.size(); i++)
		{
			Layer* layer = m_Layers[i];
			LUMOS_PROFILE(system::Profiler::OnBeginRange("Layer Update : " + layer->GetName(), true, "Update"));
			layer->OnUpdate(timeStep, scene);
			LUMOS_PROFILE(system::Profiler::OnEndRange("Layer Update : " + layer->GetName(), true, "Update"));
		}
	}

	void LayerStack::OnEvent(Event& e)
	{
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

		m_LayerCount = 0;
    }
}
