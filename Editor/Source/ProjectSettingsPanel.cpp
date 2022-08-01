#include "ProjectSettingsPanel.h"
#include "Editor.h"

#include <Lumos/Scene/Scene.h>

namespace Lumos
{
    ProjectSettingsPanel::ProjectSettingsPanel()
    {
        m_Name       = "ProjectSettings###projectsettings";
        m_SimpleName = "Project Settings";
    }

    void ProjectSettingsPanel::OnImGui()
    {
        LUMOS_PROFILE_FUNCTION();

        ImGui::Begin(m_Name.c_str(), &m_Active, 0);

        ImGui::Columns(2);
        Lumos::ImGuiUtilities::ScopedStyle(ImGuiStyleVar_FramePadding, ImVec2(2, 2));

        {
            auto& projectSettings = Application::Get().GetProjectSettings();
            ImGuiUtilities::PropertyConst("Project Name", projectSettings.m_ProjectName);
            ImGuiUtilities::PropertyConst("Project Root", projectSettings.m_ProjectRoot);
            ImGuiUtilities::PropertyConst("Engine Asset Path", projectSettings.m_EngineAssetPath);
            ImGuiUtilities::Property("App Width", (int&)projectSettings.Width, 0, 0, ImGuiUtilities::PropertyFlag::ReadOnly);
            ImGuiUtilities::Property("App Height", (int&)projectSettings.Height, 0, 0, ImGuiUtilities::PropertyFlag::ReadOnly);
            ImGuiUtilities::Property("Fullscreen", projectSettings.Fullscreen);
            ImGuiUtilities::Property("VSync", projectSettings.VSync);
            ImGuiUtilities::Property("Borderless", projectSettings.Borderless);
            ImGuiUtilities::Property("Show Console", projectSettings.ShowConsole);
            ImGuiUtilities::Property("Title", projectSettings.Title);
            ImGuiUtilities::Property("RenderAPI", projectSettings.RenderAPI, 0, 1);
            ImGuiUtilities::Property("Project Version", projectSettings.ProjectVersion, 0, 0, ImGuiUtilities::PropertyFlag::ReadOnly);
        }
        ImGui::Columns(1);
        ImGui::End();
    }
}
