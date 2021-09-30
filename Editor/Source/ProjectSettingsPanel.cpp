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
            auto& projectSettings = Application::Get().GetProjectSettings();
            ImGuiHelpers::PropertyConst("Project Name", projectSettings.m_ProjectName);
            ImGuiHelpers::PropertyConst("Project Root", projectSettings.m_ProjectRoot);
            ImGuiHelpers::PropertyConst("Engine Asset Path", projectSettings.m_EngineAssetPath);
            ImGuiHelpers::Property("App Width", (int&)projectSettings.Width, 0, 0, ImGuiHelpers::PropertyFlag::ReadOnly);
            ImGuiHelpers::Property("App Height", (int&)projectSettings.Height, 0, 0, ImGuiHelpers::PropertyFlag::ReadOnly);
            ImGuiHelpers::Property("Fullscreen", projectSettings.Fullscreen);
            ImGuiHelpers::Property("VSync", projectSettings.VSync);
            ImGuiHelpers::Property("Borderless", projectSettings.Borderless);
            ImGuiHelpers::Property("Show Console", projectSettings.ShowConsole);
            ImGuiHelpers::Property("Title", projectSettings.Title);
            ImGuiHelpers::Property("RenderAPI", projectSettings.RenderAPI, 0, 1);
            ImGuiHelpers::Property("Project Version", projectSettings.ProjectVersion, 0, 0, ImGuiHelpers::PropertyFlag::ReadOnly);
        }
        ImGui::Columns(1);
        ImGui::End();
    }
}
