#include "ApplicationInfoPanel.h"

#include <Lumos/Core/Application.h>
#include <Lumos/Scene/SceneManager.h>
#include <Lumos/Scene/Scene.h>
#include <Lumos/Core/Asset/AssetManager.h>

#include "Editor.h"
#include "PerfGraph.h"

#include <Lumos/Core/Engine.h>
#include <Lumos/Core/OS/Window.h>
#include <Lumos/Graphics/Renderers/SceneRenderer.h>
#include <Lumos/ImGui/ImGuiUtilities.h>
#include <Lumos/Utilities/StringUtilities.h>
#include <imgui/imgui.h>

namespace Lumos
{
    ApplicationInfoPanel::ApplicationInfoPanel()
    {
        m_Name       = "Application Info###appinfo";
        m_SimpleName = "Application Info";
    }

    void ApplicationInfoPanel::OnImGui()
    {
        auto flags = ImGuiWindowFlags_NoCollapse;
        if(ImGuiUtilities::BeginPanel(m_Name.c_str(), nullptr, flags))
        {
            ImGuiUtilities::PushID();

            if(ImGui::TreeNodeEx("Application", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto systems = Application::Get().GetSystemManager();

                if(ImGui::TreeNode("Quality Settings"))
                {
                    QualitySettings& qs = Application::Get().GetQualitySettings();
                    int shadowQuality   = (int)qs.ShadowQuality;
                    int shadowRes       = (int)qs.ShadowResolution;
                    float resScale      = qs.RendererScale;

                    ImGui::Columns(2);

                    if(ImGuiUtilities::Property("Render Scale", resScale, 0.01f, 3.0f, 0.1f, ImGuiUtilities::PropertyFlag::DragValue))
                        qs.RendererScale = resScale;

                    if(ImGuiUtilities::Property("Shadow Quality", shadowQuality, 0, 3))
                        qs.ShadowQuality = (ShadowQualitySetting)shadowQuality;

                    if(ImGuiUtilities::Property("Shadow Resolution", shadowRes, 0, 3))
                        qs.ShadowResolution = (ShadowResolutionSetting)shadowRes;

                    ImGui::Columns(1);

                    ImGui::TreePop();
                }

                if(ImGui::TreeNode("Systems"))
                {
                    systems->OnImGui();
                    ImGui::TreePop();
                }

                auto SceneRenderer = Application::Get().GetSceneRenderer();
                if(ImGui::TreeNode("SceneRenderer"))
                {
                    SceneRenderer->OnImGui();
                    ImGui::TreePop();
                }

                ImGui::NewLine();
                ImGui::Columns(2);
                bool VSync = Application::Get().GetWindow()->GetVSync();
                if(ImGuiUtilities::Property("VSync", VSync))
                {
                    auto editor = m_Editor;
                    Application::Get().QueueEvent([VSync, editor]
                                                  {
                                                      Application::Get().GetWindow()->SetVSync(VSync);
                                                      Application::Get().GetWindow()->GetSwapChain()->SetVSync(VSync);
                                                      Graphics::Renderer::GetRenderer()->OnResize(Application::Get().GetWindow()->GetWidth(), Application::Get().GetWindow()->GetHeight()); });
                }

                ImGui::Columns(1);

                ImGui::Text("FPS : %5.2i", Engine::Get().Statistics().FramesPerSecond);
                PerfGraph::FramerateTooltip();

                ImGui::Text("Frame Time : %5.2f ms", Engine::Get().Statistics().FrameTime);
                PerfGraph::FrametimeTooltip();

                ImGui::Text("Arena Count : %i", GetArenaCount());
                ImGui::Text("Num Draw Calls  %u", Engine::Get().Statistics().NumDrawCalls);
                ImGui::Text("Total Triangles  %u", Engine::Get().Statistics().TriangleCount);
                ImGui::Text("Num Rendered Objects %u", Engine::Get().Statistics().NumRenderedObjects);
                ImGui::Text("Num Shadow Objects %u", Engine::Get().Statistics().NumShadowObjects);
                ImGui::Text("Bound Pipelines %u", Engine::Get().Statistics().BoundPipelines);
                ImGui::Text("Bound RenderPasses %u", Engine::Get().Statistics().BoundRenderPasses);

                auto& srStats = SceneRenderer->GetSceneRendererStats();
                if(srStats.NumInstanceBatches > 0)
                {
                    ImGui::Separator();
                    ImGui::Text("Instance Batches: %u (%u objects)", srStats.NumInstanceBatches, srStats.NumInstancedObjects);
                    u32 showCount = srStats.NumInstanceBatches < Graphics::SceneRendererStats::MaxTrackedBatches
                        ? srStats.NumInstanceBatches : Graphics::SceneRendererStats::MaxTrackedBatches;
                    for(u32 b = 0; b < showCount; b++)
                    {
                        auto& bi = srStats.InstanceBatches[b];
                        if(bi.instanceCount == 0)
                            continue;
                        ImGui::Text("  %s x%u", bi.meshName ? bi.meshName : "?", bi.instanceCount);
                    }
                    if(srStats.NumInstanceBatches > Graphics::SceneRendererStats::MaxTrackedBatches)
                        ImGui::Text("  ... +%u more", srStats.NumInstanceBatches - Graphics::SceneRendererStats::MaxTrackedBatches);
                    ImGui::Separator();
                }

                if(ImGui::TreeNodeEx("Arenas", 0))
                {
                    uint64_t totalAllocated = 0;
                    for(int i = 0; i < GetArenaCount(); i++)
                    {
                        auto arena = GetArena(i);
                        totalAllocated += arena->Size;
                        float percentageFull = (float)arena->Position / (float)arena->Size;
                        ImGui::ProgressBar(percentageFull);
                        Lumos::ImGuiUtilities::Tooltip((Lumos::StringUtilities::BytesToString(arena->Position) + " / " + Lumos::StringUtilities::BytesToString(arena->Size)).c_str());
                    }
                    ImGui::Text("Total %s", Lumos::StringUtilities::BytesToString(totalAllocated).c_str());
                    ImGui::TreePop();
                }

                ImGui::Text("Scene : %s", Application::Get().GetSceneManager()->GetCurrentScene()->GetSceneName().c_str());
                ImGui::TreePop();
            }

            ImGuiUtilities::PopID();
        }
        ImGui::End();
    }
}
