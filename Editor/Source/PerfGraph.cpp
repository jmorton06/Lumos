#include "PerfGraph.h"

#include <Lumos/Core/Engine.h>
#include <imgui/imgui.h>
#include <imgui/Plugins/implot/implot.h>

namespace Lumos
{
    namespace PerfGraph
    {
        struct ScrollingBuffer
        {
            int MaxSize;
            int Offset;
            ImVector<ImVec2> Data;
            ScrollingBuffer(int max_size = 40000)
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
        };

        static ScrollingBuffer s_Framerate, s_Frametime;
        static float s_Time = 0;

        void Record()
        {
            s_Time += ImGui::GetIO().DeltaTime;
            s_Framerate.AddPoint(s_Time, ImGui::GetIO().Framerate);
            s_Frametime.AddPoint(s_Time, (float)Engine::GetTimeStep().GetMillis());
        }

        // Call right after the item. Uses BeginItemTooltip (stationary + short delay)
        // so sweeping the cursor between widgets doesn't spawn stray/duplicate tooltips.
        void FramerateTooltip()
        {
            if(s_Framerate.Data.empty() || !ImGui::BeginItemTooltip())
                return;

            if(ImPlot::BeginPlot("##FramerateTooltip", ImVec2(350, 200), ImPlotFlags_CanvasOnly))
            {
                ImPlot::SetupAxis(ImAxis_X1, nullptr, ImPlotAxisFlags_NoTickLabels);
                ImPlot::SetupAxis(ImAxis_Y1, "FPS", ImPlotAxisFlags_AutoFit);
                ImPlot::SetupAxisLimits(ImAxis_X1, s_Time - 10.0f, s_Time, ImGuiCond_Always);
                ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 120);
                ImPlot::PlotLine("##Framerate", &s_Framerate.Data[0].x, &s_Framerate.Data[0].y, s_Framerate.Data.size(), 0, s_Framerate.Offset, 2 * sizeof(float));
                ImPlot::EndPlot();
            }
            ImGui::EndTooltip();
        }

        void FrametimeTooltip()
        {
            if(s_Frametime.Data.empty() || !ImGui::BeginItemTooltip())
                return;

            if(ImPlot::BeginPlot("##FrametimeTooltip", ImVec2(350, 200), ImPlotFlags_CanvasOnly))
            {
                ImPlot::SetupAxis(ImAxis_X1, nullptr, ImPlotAxisFlags_NoTickLabels);
                ImPlot::SetupAxis(ImAxis_Y1, "Frame (ms)", ImPlotAxisFlags_AutoFit);
                ImPlot::SetupAxisLimits(ImAxis_X1, s_Time - 10.0f, s_Time, ImGuiCond_Always);
                ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 60);
                ImPlot::PlotLine("##Frametime", &s_Frametime.Data[0].x, &s_Frametime.Data[0].y, s_Frametime.Data.size(), 0, s_Frametime.Offset, 2 * sizeof(float));
                ImPlot::EndPlot();
            }
            ImGui::EndTooltip();
        }
    }
}
