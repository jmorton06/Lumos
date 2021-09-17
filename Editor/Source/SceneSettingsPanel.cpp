#include "SceneSettingsPanel.h"
#include "Editor.h"

#include <Lumos/Scene/Scene.h>

namespace Lumos
{
    SceneSettingsPanel::SceneSettingsPanel()
    {
        m_Name = "SceneSettings###scenesettings";
        m_SimpleName = "Scene Settings";
    }

    void SceneSettingsPanel::OnImGui()
    {
        LUMOS_PROFILE_FUNCTION();

        if(!m_CurrentScene)
            return;

        ImGui::Begin(m_Name.c_str(), &m_Active, 0);

        ImGui::Columns(2);
        Lumos::ImGuiHelpers::ScopedStyle(ImGuiStyleVar_FramePadding, ImVec2(2, 2));

        {
            auto sceneName = m_CurrentScene->GetSceneName();
            int sceneVersion = m_CurrentScene->GetSceneVersion();

            auto& sceneSettings = m_CurrentScene->GetSettings();

            ImGuiHelpers::Property("Name", sceneName);
            ImGuiHelpers::Property("Version", sceneVersion, ImGuiHelpers::PropertyFlag::ReadOnly);

            ImGuiHelpers::Property("Audio Enabled", sceneSettings.AudioEnabled);
            ImGuiHelpers::Property("Physics 2D Enabled", sceneSettings.PhysicsEnabled2D);
            ImGuiHelpers::Property("Physics 3D Enabled", sceneSettings.PhysicsEnabled3D);
            ImGuiHelpers::Property("Renderer 2D Enabled", sceneSettings.RenderSettings.Renderer2DEnabled);
            ImGuiHelpers::Property("Renderer 3D Enabled", sceneSettings.RenderSettings.Renderer3DEnabled);
            ImGuiHelpers::Property("Shadow Enabled", sceneSettings.RenderSettings.ShadowsEnabled);
            ImGuiHelpers::Property("Skybox Render Enabled", sceneSettings.RenderSettings.SkyboxRenderEnabled);
            ImGuiHelpers::Property("Debug Renderer Enabled", sceneSettings.RenderSettings.DebugRenderEnabled);
            auto& registry = m_CurrentScene->GetRegistry();
            int entityCount = (int)registry.size();
            ImGuiHelpers::Property("Entity Count", entityCount, ImGuiHelpers::PropertyFlag::ReadOnly);
        }
        ImGui::Columns(1);
        ImGui::End();
    }
}
