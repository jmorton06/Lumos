#include "SceneSettingsPanel.h"
#include "Editor.h"

#include <Lumos/Scene/Scene.h>
#include <Lumos/Physics/LumosPhysicsEngine/LumosPhysicsEngine.h>
#include <Lumos/Core/OS/FileSystem.h>
#include <Lumos/Core/Profiler.h>

namespace Lumos
{
    SceneSettingsPanel::SceneSettingsPanel()
    {
        m_Name       = "SceneSettings###scenesettings";
        m_SimpleName = "Scene Settings";
    }

    void SceneSettingsPanel::OnImGui()
    {
        LUMOS_PROFILE_FUNCTION();

        if(!m_CurrentScene)
            return;

        if(ImGui::Begin(m_Name.c_str(), &m_Active, 0))
        {
            Lumos::ImGuiUtilities::ScopedStyle(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            Lumos::ImGuiUtilities::PushID();
            {
                const auto& sceneName = m_CurrentScene->GetSceneName();
                int sceneVersion      = m_CurrentScene->GetSceneVersion();
                auto& sceneSettings   = m_CurrentScene->GetSettings();

                String8 nameBuffer = { 0 };
                nameBuffer.str     = PushArray(m_Editor->GetFrameArena(), uint8_t, INPUT_BUF_SIZE);
                nameBuffer.size    = INPUT_BUF_SIZE;

                MemoryCopy(nameBuffer.str, sceneName.c_str(), sceneName.size());

                ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();
                ImGui::PushItemWidth(contentRegionAvailable.x * 0.5f);

                {
                    ImGuiUtilities::ScopedFont boldFont(ImGui::GetIO().Fonts->Fonts[1]);
                    if(ImGui::InputText("##Name", (char*)nameBuffer.str, INPUT_BUF_SIZE, 0))
                    {
                        if(!m_NameUpdated)
                            m_SceneName = m_CurrentScene->GetSceneName();
                        m_NameUpdated = true;

                        m_CurrentScene->SetName((char*)nameBuffer.str);
                    }

                    if(!ImGui::IsItemActive() && m_NameUpdated)
                    {
                        m_NameUpdated = false;
                        std::string scenePath;
                        if(FileSystem::Get().ResolvePhysicalPath("//Assets/Scenes/" + m_SceneName + ".lsn", scenePath))
                        {
                            std::filesystem::rename(scenePath, m_Editor->GetProjectSettings().m_ProjectRoot + "Assets/Scenes/" + m_CurrentScene->GetSceneName() + ".lsn");
                        }

                        // Save project with updated scene name
                        m_Editor->Serialise();
                    }
                }

                ImGui::SameLine();
                ImGui::Text("Version : %i", (int)sceneVersion);

                ImGui::Columns(1);
                if(ImGui::CollapsingHeader("Renderer", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::Indent();

                    if(ImGui::TreeNodeEx("Renderers", ImGuiTreeNodeFlags_Framed))
                    {
                        ImGui::Columns(2);
                        ImGuiUtilities::Property("Renderer 2D Enabled", sceneSettings.RenderSettings.Renderer2DEnabled);
                        ImGuiUtilities::Property("Renderer 3D Enabled", sceneSettings.RenderSettings.Renderer3DEnabled);
                        ImGuiUtilities::Property("Shadow Enabled", sceneSettings.RenderSettings.ShadowsEnabled);
                        ImGuiUtilities::Property("Skybox Render Enabled", sceneSettings.RenderSettings.SkyboxRenderEnabled);
                        ImGuiUtilities::Property("Skybox Mip Level", sceneSettings.RenderSettings.SkyboxMipLevel, 0.0f, 14.0f, 0.01f);
                        ImGuiUtilities::Property("Debug Renderer Enabled", sceneSettings.RenderSettings.DebugRenderEnabled);
                        ImGui::Columns(1);
                        ImGui::TreePop();
                    }
                    if(ImGui::TreeNodeEx("Post Process", ImGuiTreeNodeFlags_Framed))
                    {

                        auto postprocessSetting = [&](const char* name, const char* checkboxname, bool& value, bool leaf) -> bool
                        {
                            ImGui::Columns(1);
                            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_SpanAvailWidth;
                            if(leaf)
                            {
                                flags |= ImGuiTreeNodeFlags_Leaf;
                            }

                            bool open = ImGui::TreeNodeEx(name, flags);
                            ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - ImGui::CalcTextSize("AA").x * 2.0f);
                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.7f, 0.7f, 0.0f));
                            ImGui::Checkbox(checkboxname, &value);
                            ImGui::PopStyleColor();

                            if(leaf)
                                ImGui::TreePop();
                            ImGui::Columns(2);
                            return open;
                        };

                        ImGui::Columns(2);
                        postprocessSetting("FXAA", "##FXAA", sceneSettings.RenderSettings.FXAAEnabled, true);
                        postprocessSetting("Debanding", "##Debanding", sceneSettings.RenderSettings.DebandingEnabled, true);
                        postprocessSetting("Chromatic Aberation", "##ChromaticAberation", sceneSettings.RenderSettings.ChromaticAberationEnabled, true);
                        postprocessSetting("Filmic Grain", "##FilmicGrain", sceneSettings.RenderSettings.FilmicGrainEnabled, true);
                        postprocessSetting("Sharpen", "##Sharpen", sceneSettings.RenderSettings.SharpenEnabled, true);

                        bool open = postprocessSetting("Bloom", "##Bloom", sceneSettings.RenderSettings.BloomEnabled, false);
                        if(open)
                        {
                            ImGuiUtilities::Property("Bloom Intensity", sceneSettings.RenderSettings.m_BloomIntensity, -1.0f, -1.0f, 0.1f);
                            ImGuiUtilities::Property("Bloom Upsample Scale", sceneSettings.RenderSettings.BloomUpsampleScale, -1.0f, -1.0f, 0.1f);
                            ImGuiUtilities::Property("Bloom Knee", sceneSettings.RenderSettings.BloomKnee, -1.0f, -1.0f, 0.1f);
                            ImGuiUtilities::Property("Bloom Threshold", sceneSettings.RenderSettings.BloomThreshold, -1.0f, -1.0f, 0.1f);
                            ImGui::TreePop();
                        }

                        open = postprocessSetting("Depth Of Field", "##DepthOfField", sceneSettings.RenderSettings.DepthOfFieldEnabled, false);
                        if(open)
                        {
                            ImGuiUtilities::Property("DOF Strength", sceneSettings.RenderSettings.DepthOfFieldStrength);
                            ImGuiUtilities::Property("DOF Distance", sceneSettings.RenderSettings.DepthOfFieldDistance);
                            ImGui::TreePop();
                        }

                        ImGui::BeginDisabled();
                        open = postprocessSetting("SSAO", "##SSAO", sceneSettings.RenderSettings.SSAOEnabled, false);
                        if(open)
                        {
                            ImGuiUtilities::Property("SSAO Sample Radius", sceneSettings.RenderSettings.SSAOSampleRadius, 0.0f, 16.0f, 0.01f);
                            ImGuiUtilities::Property("SSAO Blur Radius", sceneSettings.RenderSettings.SSAOBlurRadius, 0, 16);
                            ImGuiUtilities::Property("SSAO Blur Enabled", sceneSettings.RenderSettings.SSAOBlur);
                            ImGuiUtilities::Property("SSAO Strength", sceneSettings.RenderSettings.SSAOStrength, 0.0f, 16.0f, 0.01f);
                            ImGui::TreePop();
                        }

                        ImGui::EndDisabled();
                        ImGui::Columns(1);
                        ImGui::TreePop();
                    }

                    static const char* toneMaps[7] = {
                        "None",
                        "Linear",
                        "Simple Reinhard",
                        "Luma Reinhard",
                        "White Preserved Luma Reinard",
                        "Uncharted 2",
                        "Aces"
                    };

                    ImGui::Columns(2);
                    ImGuiUtilities::PropertyDropdown("ToneMap", toneMaps, 7, (int*)&m_CurrentScene->GetSettings().RenderSettings.m_ToneMapIndex);
                    ImGuiUtilities::Property("Brightness", sceneSettings.RenderSettings.Brightness, -1.0f, 1.0f, 0.01f);
                    ImGuiUtilities::Property("Contrast", sceneSettings.RenderSettings.Contrast, 0.0f, 2.0f, 0.01f);
                    ImGuiUtilities::Property("Saturation", sceneSettings.RenderSettings.Saturation, 0.0f, 1.0f, 0.01f);
                    ImGui::Separator();

                    auto& registry  = m_CurrentScene->GetRegistry();
                    int entityCount = (int)registry.storage<entt::entity>().size();
                    ImGuiUtilities::Property("Entity Count", entityCount, 0, 0, ImGuiUtilities::PropertyFlag::ReadOnly);

                    static const char* debugModes[7] = {
                        "None",
                        "SSAO",
                        "SSAO1",
                        "Normals",
                        "Bloom",
                        "Noise",
                        "Post Process"
                    };
                    ImGuiUtilities::PropertyDropdown("Debug View Mode", debugModes, 7, (int*)&sceneSettings.RenderSettings.DebugMode);
                    ImGui::Columns(1);
                    ImGui::Unindent();
                }

                ImGui::Columns(1);
                if(ImGui::CollapsingHeader("Systems"))
                {
                    ImGui::Columns(2);
                    ImGuiUtilities::Property("Audio Enabled", sceneSettings.AudioEnabled);
                    ImGuiUtilities::Property("Physics 2D Enabled", sceneSettings.PhysicsEnabled2D);
                    ImGuiUtilities::Property("Physics 3D Enabled", sceneSettings.PhysicsEnabled3D);
                }

                ImGui::Columns(1);
                if(ImGui::CollapsingHeader("Physics3D"))
                {
                    ImGui::Columns(2);
                    auto physicsSystem = Application::Get().GetSystem<LumosPhysicsEngine>();

                    if(physicsSystem)
                    {
                        if(ImGuiUtilities::Property("Dampening", sceneSettings.Physics3DSettings.Dampening))
                            physicsSystem->SetDampingFactor(sceneSettings.Physics3DSettings.Dampening);
                        if(ImGuiUtilities::Property("Max Updates Per Frame", sceneSettings.Physics3DSettings.m_MaxUpdatesPerFrame))
                            physicsSystem->SetMaxUpdatesPerFrame(sceneSettings.Physics3DSettings.m_MaxUpdatesPerFrame);
                        if(ImGuiUtilities::Property("Position Iterations", sceneSettings.Physics3DSettings.PositionIterations))
                            physicsSystem->SetPositionIterations(sceneSettings.Physics3DSettings.PositionIterations);
                        if(ImGuiUtilities::Property("Velocity Iterations", sceneSettings.Physics3DSettings.VelocityIterations))
                            physicsSystem->SetPositionIterations(sceneSettings.Physics3DSettings.VelocityIterations);
                        if(ImGuiUtilities::Property("Gravity", sceneSettings.Physics3DSettings.Gravity))
                            physicsSystem->SetGravity(sceneSettings.Physics3DSettings.Gravity);

                        if(ImGuiUtilities::Property("BroadPhaseIndex", sceneSettings.Physics3DSettings.BroadPhaseTypeIndex))
                            physicsSystem->SetBroadphaseType((BroadphaseType)sceneSettings.Physics3DSettings.BroadPhaseTypeIndex);
                        if(ImGuiUtilities::Property("IntegrationType", sceneSettings.Physics3DSettings.IntegrationTypeIndex))
                            physicsSystem->SetIntegrationType((IntegrationType)sceneSettings.Physics3DSettings.IntegrationTypeIndex);
                    }
                }

                ImGui::Columns(1);
                if(ImGui::CollapsingHeader("Physics2D"))
                {
                    ImGui::Columns(2);
                    ImGuiUtilities::Property("Dampening", sceneSettings.Physics2DSettings.Dampening);
                    ImGuiUtilities::Property("Position Iterations", sceneSettings.Physics2DSettings.PositionIterations);
                    ImGuiUtilities::Property("Velocity Iterations", sceneSettings.Physics2DSettings.VelocityIterations);
                    ImGuiUtilities::Property("Gravity", sceneSettings.Physics2DSettings.Gravity);
                    ImGuiUtilities::Property("IntegrationType", sceneSettings.Physics3DSettings.IntegrationTypeIndex);
                }
            }
            ImGui::Columns(1);
            Lumos::ImGuiUtilities::PopID();
        }
        ImGui::End();
    }
}
