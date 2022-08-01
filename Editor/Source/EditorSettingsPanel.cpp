#include "EditorSettingsPanel.h"
#include "Editor.h"

#include <Lumos/Scene/Scene.h>
#include <Lumos/Core/Version.h>
namespace Lumos
{
    EditorSettingsPanel::EditorSettingsPanel()
    {
        m_Name       = "EditorSettings###editorsettings";
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
        Lumos::ImGuiUtilities::ScopedStyle(ImGuiStyleVar_FramePadding, ImVec2(2, 2));

        {
            auto sceneName   = m_CurrentScene->GetSceneName();
            int sceneVersion = m_CurrentScene->GetSceneVersion();

            auto& editorSettings = m_Editor->GetSettings();

            ImGuiUtilities::Property("Grid Size", editorSettings.m_GridSize);
            ImGuiUtilities::Property("Grid Enabled", editorSettings.m_ShowGrid);
            ImGuiUtilities::Property("Gizmos Enabled", editorSettings.m_ShowGizmos);
            ImGuiUtilities::Property("ImGuizmo Scale", editorSettings.m_ImGuizmoScale);
            ImGuiUtilities::Property("Show View Selected", editorSettings.m_ShowViewSelected);
            ImGuiUtilities::Property("View 2D", editorSettings.m_View2D);
            ImGuiUtilities::Property("Fullscreen on play", editorSettings.m_FullScreenOnPlay);
            ImGuiUtilities::Property("Snap Amount", editorSettings.m_SnapAmount);
            ImGuiUtilities::Property("Sleep Out of Focus", editorSettings.m_SleepOutofFocus);
            ImGuiUtilities::Property("Theme", (int&)editorSettings.m_Theme);
            ImGuiUtilities::Property("Debug draw flags", (int&)editorSettings.m_DebugDrawFlags);
            ImGuiUtilities::Property("Physics 2D debug flags", (int&)editorSettings.m_Physics2DDebugFlags);
            ImGuiUtilities::Property("Physics 3D debug flags", (int&)editorSettings.m_Physics3DDebugFlags);
        }
        ImGui::Columns(1);

        if(ImGui::Button("Save Settings"))
            m_Editor->SaveEditorSettings();
        ImGui::End();
    }
}
