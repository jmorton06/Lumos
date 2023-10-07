#include "ApplicationInfoPanel.h"

#include <Lumos/Core/Application.h>
#include <Lumos/Scene/SceneManager.h>
#include <Lumos/Scene/Scene.h>

#include <Lumos/Core/Engine.h>
#include <Lumos/Core/OS/Window.h>
#include <Lumos/Graphics/Renderers/RenderPasses.h>
#include <Lumos/ImGui/ImGuiUtilities.h>
#include <imgui/imgui.h>
#include <imgui/Plugins/implot/implot.h>

namespace Lumos
{
    struct ScrollingBuffer
    {
        int MaxSize;
        int Offset;
        ImVector<ImVec2> Data;
        ScrollingBuffer(int max_size = 2000)
        {
            MaxSize = max_size;
            Offset  = 0;
            Data.reserve(MaxSize);
        }
        void AddPoint(float x, float y)
        {
            if(Data.size() < MaxSize)
                Data.push_back(ImVec2(x, y));
            else
            {
                Data[Offset] = ImVec2(x, y);
                Offset       = (Offset + 1) % MaxSize;
            }
        }
        void Erase()
        {
            if(Data.size() > 0)
            {
                Data.shrink(0);
                Offset = 0;
            }
        }
    };

    ApplicationInfoPanel::ApplicationInfoPanel()
    {
        m_Name       = "ApplicationInfo";
        m_SimpleName = "ApplicationInfo";
    }

    static float MaxFrameTime = 0;
    void ApplicationInfoPanel::OnImGui()
    {
        auto flags = ImGuiWindowFlags_NoCollapse;
        if(ImGui::Begin(m_Name.c_str(), &m_Active, flags))
        {
            ImGuiUtilities::PushID();

            // m_FPSData.push_back(Lumos::Engine::Get().Statistics().FramesPerSecond);
            // MaxFrameTime = Maths::Max(MaxFrameTime, m_FPSData.back());

            static ScrollingBuffer rdata(40000), rdata1(40000);
            static float t = 0;
            t += ImGui::GetIO().DeltaTime;
            static int frame = 0;
            frame++;

            // if (frame > (int)(ImGui::GetIO().Framerate / 60))
            {
                rdata.AddPoint(t, ImGui::GetIO().Framerate);
                rdata1.AddPoint(t, (float)Lumos::Engine::GetTimeStep().GetMillis()); // 1000.0f / ImGui::GetIO().Framerate);
            }

            static ImPlotAxisFlags rt_axis = ImPlotAxisFlags_NoTickLabels;
            static bool PlotFrameTime      = true;
            static bool PlotFramerate      = false;

            ImGui::Checkbox("Plot Frame Time", &PlotFrameTime);
            ImGui::Checkbox("Plot Frame Rate", &PlotFramerate);

            if(PlotFramerate && ImPlot::BeginPlot("Framerate", ImVec2(-1, 350), 0))
            {
                ImPlot::SetupAxis(ImAxis_X1, nullptr, rt_axis);
                ImPlot::SetupAxis(ImAxis_Y1, "FPS", 0);
                ImPlot::SetupAxisLimits(ImAxis_X1, t - 10.0f, t, ImGuiCond_Always);
                ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 120);

                ImPlot::PlotLine("##Framerate", &rdata.Data[0].x, &rdata.Data[0].y, rdata.Data.size(), 0, rdata.Offset, 2 * sizeof(float));

                ImPlot::EndPlot();
            }
            if(PlotFrameTime && ImPlot::BeginPlot("Frametime", ImVec2(-1, 350), 0))
            {
                ImPlot::SetupAxis(ImAxis_X1, nullptr, rt_axis);
                ImPlot::SetupAxis(ImAxis_Y1, "Frame (ms)", 0);
                ImPlot::SetupAxisLimits(ImAxis_X1, t - 10.0f, t, ImGuiCond_Always);
                ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 60);

                ImPlot::PlotLine("##Framerate", &rdata1.Data[0].x, &rdata1.Data[0].y, rdata1.Data.size(), 0, rdata1.Offset, 2 * sizeof(float));

                ImPlot::EndPlot();
            }

            if(ImGui::TreeNodeEx("Application", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto systems = Application::Get().GetSystemManager();

                if(ImGui::TreeNode("Systems"))
                {
                    systems->OnImGui();
                    ImGui::TreePop();
                }

                auto RenderPasses = Application::Get().GetRenderPasses();
                if(ImGui::TreeNode("RenderPasses"))
                {
                    RenderPasses->OnImGui();
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
                ImGui::Text("UPS : %5.2i", Engine::Get().Statistics().UpdatesPerSecond);
                ImGui::Text("Frame Time : %5.2f ms", Engine::Get().Statistics().FrameTime);
                ImGui::Text("Arena Count : %i", GetArenaCount());
                ImGui::NewLine();
                ImGui::Text("Scene : %s", Application::Get().GetSceneManager()->GetCurrentScene()->GetSceneName().c_str());
                ImGui::TreePop();
            };

            ImGuiUtilities::PopID();
        }
        ImGui::End();
    }
}
