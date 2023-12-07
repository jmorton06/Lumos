#include "ProjectSettingsPanel.h"
#include "Editor.h"

#include <Lumos/Core/Profiler.h>

namespace Lumos
{
    ProjectSettingsPanel::ProjectSettingsPanel()
    {
        m_Name       = "Project Settings###projectsettings";
        m_SimpleName = "Project Settings";
    }

    void ProjectSettingsPanel::OnImGui()
    {
        LUMOS_PROFILE_FUNCTION();

        ImGui::Begin(m_Name.c_str(), &m_Active, 0);
        ImGuiUtilities::PushID();

        ImGui::Columns(2);
        Lumos::ImGuiUtilities::ScopedStyle(ImGuiStyleVar_FramePadding, ImVec2(2, 2));

        {
            auto& projectSettings = Application::Get().GetProjectSettings();
            auto projectName      = projectSettings.m_ProjectName;

            if(m_NameUpdated)
                projectName = m_ProjectName;

            if(ImGuiUtilities::Property("Project Name", projectName, ImGuiUtilities::PropertyFlag::None))
            {
                m_NameUpdated = true;
            }

            ImGuiUtilities::PropertyConst("Project Root", projectSettings.m_ProjectRoot.c_str());
            ImGuiUtilities::PropertyConst("Engine Asset Path", projectSettings.m_EngineAssetPath.c_str());
            ImGuiUtilities::Property("App Width", (int&)projectSettings.Width, 0, 0, ImGuiUtilities::PropertyFlag::ReadOnly);
            ImGuiUtilities::Property("App Height", (int&)projectSettings.Height, 0, 0, ImGuiUtilities::PropertyFlag::ReadOnly);
            ImGuiUtilities::Property("Fullscreen", projectSettings.Fullscreen);
            ImGuiUtilities::Property("VSync", projectSettings.VSync);
            ImGuiUtilities::Property("Borderless", projectSettings.Borderless);
            ImGuiUtilities::Property("Show Console", projectSettings.ShowConsole);
            ImGuiUtilities::Property("Title", projectSettings.Title);
            ImGuiUtilities::Property("RenderAPI", projectSettings.RenderAPI, 0, 1);
            ImGuiUtilities::Property("Project Version", projectSettings.ProjectVersion, 0, 0, ImGuiUtilities::PropertyFlag::ReadOnly);

            if(!ImGui::IsItemActive() && m_NameUpdated)
            {
                m_NameUpdated = false;
                auto fullPath = projectSettings.m_ProjectRoot + projectSettings.m_ProjectName + std::string(".lmproj");
                if(std::filesystem::exists(fullPath))
                {
                    projectSettings.m_ProjectName = projectName;
                    std::filesystem::rename(fullPath, projectSettings.m_ProjectRoot + projectSettings.m_ProjectName + std::string(".lmproj"));
                }
                else
                    projectSettings.m_ProjectName = projectName;
            }
        }
        ImGui::Columns(1);
        ImGuiUtilities::PopID();
        ImGui::End();
    }
}
