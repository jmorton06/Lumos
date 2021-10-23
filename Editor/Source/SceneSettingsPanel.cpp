#include "SceneSettingsPanel.h"
#include "Editor.h"

#include <Lumos/Scene/Scene.h>
#include <Lumos/Physics/LumosPhysicsEngine/LumosPhysicsEngine.h>

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

        Lumos::ImGuiHelpers::ScopedStyle(ImGuiStyleVar_FramePadding, ImVec2(2, 2));

        {
            auto sceneName = m_CurrentScene->GetSceneName();
            int sceneVersion = m_CurrentScene->GetSceneVersion();
            auto& sceneSettings = m_CurrentScene->GetSettings();

            ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();
            ImGui::PushItemWidth(contentRegionAvailable.x * 0.5f);

            {
                ImGuiHelpers::ScopedFont boldFont(ImGui::GetIO().Fonts->Fonts[1]);
                if(ImGuiHelpers::InputText(sceneName))
                {
                    m_CurrentScene->SetName(sceneName);
                }
            }

            ImGui::SameLine();
            ImGui::Text("Version : %i", (int)sceneVersion);

            ImGui::Columns(1);
            if(ImGui::CollapsingHeader("Systems"))
            {
                ImGui::Columns(2);
                ImGuiHelpers::Property("Audio Enabled", sceneSettings.AudioEnabled);
                ImGuiHelpers::Property("Physics 2D Enabled", sceneSettings.PhysicsEnabled2D);
                ImGuiHelpers::Property("Physics 3D Enabled", sceneSettings.PhysicsEnabled3D);
            }

            ImGui::Columns(1);
            if(ImGui::CollapsingHeader("Renderer"))
            {
                ImGui::Columns(2);
                ImGuiHelpers::Property("Renderer 2D Enabled", sceneSettings.RenderSettings.Renderer2DEnabled);
                ImGuiHelpers::Property("Renderer 3D Enabled", sceneSettings.RenderSettings.Renderer3DEnabled);
                ImGuiHelpers::Property("Shadow Enabled", sceneSettings.RenderSettings.ShadowsEnabled);
                ImGuiHelpers::Property("Skybox Render Enabled", sceneSettings.RenderSettings.SkyboxRenderEnabled);
                ImGuiHelpers::Property("Debug Renderer Enabled", sceneSettings.RenderSettings.DebugRenderEnabled);
                auto& registry = m_CurrentScene->GetRegistry();
                int entityCount = (int)registry.size();
                ImGuiHelpers::Property("Entity Count", entityCount, 0, 0, ImGuiHelpers::PropertyFlag::ReadOnly);
            }

            ImGui::Columns(1);
            if(ImGui::CollapsingHeader("Physics3D"))
            {
                ImGui::Columns(2);
                auto physicsSystem = Application::Get().GetSystem<LumosPhysicsEngine>();

                if(physicsSystem)
                {
                    if(ImGuiHelpers::Property("Dampening", sceneSettings.Physics3DSettings.Dampening))
                        physicsSystem->SetDampingFactor(sceneSettings.Physics3DSettings.Dampening);
                    if(ImGuiHelpers::Property("Max Updates Per Frame", sceneSettings.Physics3DSettings.m_MaxUpdatesPerFrame))
                        physicsSystem->SetMaxUpdatesPerFrame(sceneSettings.Physics3DSettings.m_MaxUpdatesPerFrame);
                    if(ImGuiHelpers::Property("Position Iterations", sceneSettings.Physics3DSettings.PositionIterations))
                        physicsSystem->SetPositionIterations(sceneSettings.Physics3DSettings.PositionIterations);
                    if(ImGuiHelpers::Property("Velocity Iterations", sceneSettings.Physics3DSettings.VelocityIterations))
                        physicsSystem->SetPositionIterations(sceneSettings.Physics3DSettings.VelocityIterations);
                    if(ImGuiHelpers::Property("Gravity", sceneSettings.Physics3DSettings.Gravity))
                        physicsSystem->SetGravity(sceneSettings.Physics3DSettings.Gravity);

                    
                    if(ImGuiHelpers::Property("BroadPhaseIndex", sceneSettings.Physics3DSettings.BroadPhaseTypeIndex))
                        physicsSystem->SetDampingFactor(sceneSettings.Physics3DSettings.BroadPhaseTypeIndex);
                    if(ImGuiHelpers::Property("IntegrationType", sceneSettings.Physics3DSettings.IntegrationTypeIndex))
                        physicsSystem->SetIntegrationType((IntegrationType)sceneSettings.Physics3DSettings.IntegrationTypeIndex);
                }
            }

            ImGui::Columns(1);
            if(ImGui::CollapsingHeader("Physics2D"))
            {
                ImGui::Columns(2);
                ImGuiHelpers::Property("Dampening", sceneSettings.Physics2DSettings.Dampening);
                ImGuiHelpers::Property("Position Iterations", sceneSettings.Physics2DSettings.PositionIterations);
                ImGuiHelpers::Property("Velocity Iterations", sceneSettings.Physics2DSettings.VelocityIterations);
                ImGuiHelpers::Property("Gravity", sceneSettings.Physics2DSettings.Gravity);
                ImGuiHelpers::Property("IntegrationType", sceneSettings.Physics3DSettings.IntegrationTypeIndex);
            }
        }
        ImGui::Columns(1);
        ImGui::End();
    }
}
