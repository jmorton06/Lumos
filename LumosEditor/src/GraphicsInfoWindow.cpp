#include "Precompiled.h"
#include "GraphicsInfoWindow.h"
#include "Graphics/API/GraphicsContext.h"

#include <imgui/imgui.h>

namespace Lumos
{
	GraphicsInfoWindow::GraphicsInfoWindow()
	{
		m_Name = "GraphicsInfo";
		m_SimpleName = "GraphicsInfo";
	}

	void GraphicsInfoWindow::OnImGui()
	{
		auto flags = ImGuiWindowFlags_NoCollapse;
		ImGui::Begin("GraphicsInfo", &m_Active, flags);
		{
			Graphics::GraphicsContext::GetContext()->OnImGui();
		}
		ImGui::End();
	}
}