#include "EditorSettingsPanel.h"
#include "Editor.h"
#include <Lumos/Maths/MathsUtilities.h>
#include <Lumos/Scene/Scene.h>
#include <Lumos/Core/Version.h>
#include <glm/gtx/matrix_decompose.hpp>
#include <Lumos/Core/Profiler.h>
#include <Lumos/ImGui/ImGuiManager.h>
#include <Lumos/Graphics/RHI/IMGUIRenderer.h>
#include <Lumos/Graphics/RHI/Renderer.h>
#include <Lumos/Graphics/RHI/Texture.h>
#include <Lumos/Graphics/RHI/GraphicsContext.h>

namespace Lumos
{
    EditorSettingsPanel::EditorSettingsPanel()
    {
        m_Name       = "Editor Settings###editorsettings";
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

            ImGui::TextUnformatted("Camera Transform");
            ImGui::Columns(1);
            // Camera Transform;
            auto& transform = m_Editor->GetEditorCameraTransform();

            auto rotation   = glm::degrees(glm::eulerAngles(transform.GetLocalOrientation()));
            auto position   = transform.GetLocalPosition();
            auto scale      = transform.GetLocalScale();
            float itemWidth = (ImGui::GetContentRegionAvail().x - (ImGui::GetFontSize() * 3.0f)) / 3.0f;

            // Call this to fix alignment with columns
            ImGui::AlignTextToFramePadding();

            if(Lumos::ImGuiUtilities::PropertyTransform("Position", position, itemWidth))
                transform.SetLocalPosition(position);

            ImGui::SameLine();
            if(Lumos::ImGuiUtilities::PropertyTransform("Rotation", rotation, itemWidth))
            {
                float pitch = Lumos::Maths::Min(rotation.x, 89.9f);
                pitch       = Lumos::Maths::Max(pitch, -89.9f);
                transform.SetLocalOrientation(glm::quat(glm::radians(glm::vec3(pitch, rotation.y, rotation.z))));
            }

            ImGui::SameLine();
            if(Lumos::ImGuiUtilities::PropertyTransform("Scale", scale, itemWidth))
            {
                transform.SetLocalScale(scale);
            }

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

            if(m_Editor->GetEditorScriptsPath().size)
                ImGuiUtilities::PropertyConst("Editor Scripts Path", (const char*)m_Editor->GetEditorScriptsPath().str);
        }
        ImGui::Columns(1);

        // Engine Asset Path;
        // Editor Script Path;

        if(ImGui::Button("Set Editor Scripts Path", ImVec2(ImGui::GetContentRegionAvail().x, 0.0f)))
        {
            Lumos::Editor::GetEditor()->GetFileBrowserPanel().Open();
            Lumos::Editor::GetEditor()->GetFileBrowserPanel().SetOpenDirectory(true);
            Lumos::Editor::GetEditor()->GetFileBrowserPanel().SetCallback(std::bind(&Lumos::Editor::SetEditorScriptsPath, Lumos::Editor::GetEditor(), std::placeholders::_1));
        }

        if(ImGui::Button("Save Settings"))
            m_Editor->SaveEditorSettings();

        ImGuiUtilities::PopID();
        ImGui::End();
    }
}
