#include "GraphicsInfoPanel.h"
#include <Lumos/Graphics/RHI/GraphicsContext.h>
#include <Lumos/Graphics/RHI/Renderer.h>

#include <imgui/imgui.h>

namespace Lumos
{
    GraphicsInfoPanel::GraphicsInfoPanel()
    {
        m_Name       = "Graphics Info##GraphicsInfo";
        m_SimpleName = "Graphics Info";
    }

    void GraphicsInfoPanel::OnImGui()
    {
        auto flags = ImGuiWindowFlags_NoCollapse;
        ImGui::Begin("GraphicsInfo", &m_Active, flags);
        {
            Graphics::Renderer::GetGraphicsContext()->OnImGui();
        }
        ImGui::End();
    }
}
