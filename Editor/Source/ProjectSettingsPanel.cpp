#include "ProjectSettingsPanel.h"
#include "Editor.h"

#include <Lumos/Scene/Scene.h>

namespace Lumos
{
    ProjectSettingsPanel::ProjectSettingsPanel()
    {
        m_Name = "ProjectSettings###projectsettings";
        m_SimpleName = "Project Settings";
    }

    void ProjectSettingsPanel::OnImGui()
    {
        LUMOS_PROFILE_FUNCTION();

        ImGui::Begin(m_Name.c_str(), &m_Active, 0);

        ImGui::Columns(2);
        Lumos::ImGuiHelpers::ScopedStyle(ImGuiStyleVar_FramePadding, ImVec2(2, 2));

        {
            ImGuiHelpers::PropertyConst("Project Name", Application::Get().GetProjectName());
            ImGuiHelpers::PropertyConst("Project Root", Application::Get().GetProjectRoot());
        }
        ImGui::Columns(1);
        ImGui::End();
    }
}
