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
        ImGuiUtilities::PushID();

        auto& version = Lumos::LumosVersion;
        ImGui::Text("Lumos Engine Version : %d.%d.%d", version.major, version.minor, version.patch);
        ImGui::Separator();
        ImGui::Columns(2);

        {
            Lumos::ImGuiUtilities::ScopedStyle frameStyle(ImGuiStyleVar_FramePadding, ImVec2(2, 2));

            auto sceneName   = m_CurrentScene->GetSceneName();
            int sceneVersion = m_CurrentScene->GetSceneVersion();

            auto& editorSettings = m_Editor->GetSettings();
            ImGuiUtilities::Property("Theme", (int&)editorSettings.m_Theme);
            ImGui::Columns(1);
            ImGui::TextUnformatted("Camera Settings");
            ImGui::Columns(2);

            if(ImGuiUtilities::Property("Camera Speed", editorSettings.m_CameraSpeed))
                m_Editor->GetEditorCameraController().SetSpeed(editorSettings.m_CameraSpeed);
            if(ImGuiUtilities::Property("Camera Near", editorSettings.m_CameraNear))
                m_Editor->GetCamera()->SetNear(editorSettings.m_CameraNear);
            if(ImGuiUtilities::Property("Camera Far", editorSettings.m_CameraFar))
                m_Editor->GetCamera()->SetFar(editorSettings.m_CameraFar);

            ImGui::Separator();

            ImGui::Columns(1);
            ImGui::TextUnformatted("Scene Panel Settings");
            ImGui::Columns(2);

            ImGuiUtilities::Property("Grid Size", editorSettings.m_GridSize);
            ImGuiUtilities::Property("Grid Enabled", editorSettings.m_ShowGrid);
            ImGuiUtilities::Property("Gizmos Enabled", editorSettings.m_ShowGizmos);
            ImGuiUtilities::Property("ImGuizmo Scale", editorSettings.m_ImGuizmoScale);
            ImGuiUtilities::Property("Show View Selected", editorSettings.m_ShowViewSelected);
            ImGuiUtilities::Property("View 2D", editorSettings.m_View2D);
            ImGuiUtilities::Property("Fullscreen on play", editorSettings.m_FullScreenOnPlay);
            ImGuiUtilities::Property("Snap Amount", editorSettings.m_SnapAmount);
            ImGuiUtilities::Property("Sleep Out of Focus", editorSettings.m_SleepOutofFocus);
            ImGuiUtilities::Property("Debug draw flags", (int&)editorSettings.m_DebugDrawFlags);
            ImGuiUtilities::Property("Physics 2D debug flags", (int&)editorSettings.m_Physics2DDebugFlags);
            ImGuiUtilities::Property("Physics 3D debug flags", (int&)editorSettings.m_Physics3DDebugFlags);
        }
        ImGui::Columns(1);

        if(ImGui::Button("Save Settings"))
            m_Editor->SaveEditorSettings();

        ImGuiUtilities::PopID();
        ImGui::End();
    }
}
