#include "EditorSettingsPanel.h"
#include "Editor.h"

#include <Lumos/Scene/Scene.h>
#include <Lumos/Core/Version.h>
namespace Lumos
{
    EditorSettingsPanel::EditorSettingsPanel()
    {
        m_Name = "EditorSettings###editorsettings";
        m_SimpleName = "Editor Settings";
    }

    void EditorSettingsPanel::OnImGui()
    {
        LUMOS_PROFILE_FUNCTION();

        if(!m_CurrentScene)
            return;

        ImGui::Begin(m_Name.c_str(), &m_Active, 0);
        auto& version = Lumos::LumosVersion;
        ImGui::Text("Lumos Engine Version : %d.%d.%d", version.major, version.minor, version.patch);
        ImGui::Separator();
        ImGui::Columns(2);
        Lumos::ImGuiHelpers::ScopedStyle(ImGuiStyleVar_FramePadding, ImVec2(2, 2));

        {
            auto sceneName = m_CurrentScene->GetSceneName();
            int sceneVersion = m_CurrentScene->GetSceneVersion();

            auto& editorSettings = m_Editor->GetSettings();

            ImGuiHelpers::Property("Grid Size", editorSettings.m_GridSize);
            ImGuiHelpers::Property("Grid Enabled", editorSettings.m_ShowGrid);
            ImGuiHelpers::Property("Gizmos Enabled", editorSettings.m_ShowGizmos);
            ImGuiHelpers::Property("ImGuizmo Scale", editorSettings.m_ImGuizmoScale);
            ImGuiHelpers::Property("Show View Selected", editorSettings.m_ShowViewSelected);
            ImGuiHelpers::Property("View 2D", editorSettings.m_View2D);
            ImGuiHelpers::Property("Fullscreen on play", editorSettings.m_FullScreenOnPlay);
            ImGuiHelpers::Property("Snap Amount", editorSettings.m_SnapAmount);
            ImGuiHelpers::Property("Sleep Out of Focus", editorSettings.m_SleepOutofFocus);
            ImGuiHelpers::Property("Theme", (int&)editorSettings.m_Theme);
            ImGuiHelpers::Property("Debug draw flags", (int&)editorSettings.m_DebugDrawFlags);
            ImGuiHelpers::Property("Physics 2D debug flags", (int&)editorSettings.m_Physics2DDebugFlags);
            ImGuiHelpers::Property("Physics 3D debug flags", (int&)editorSettings.m_Physics3DDebugFlags);
        }
        ImGui::Columns(1);

        if(ImGui::Button("Save Settings"))
            m_Editor->SaveEditorSettings();
        ImGui::End();
    }
}
