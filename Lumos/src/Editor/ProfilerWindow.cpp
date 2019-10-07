#include "lmpch.h"
#include "ProfilerWindow.h"
#include "App/Engine.h"

#include <imgui/imgui.h>
#include <IconFontCppHeaders/IconsFontAwesome5.h>

namespace Lumos
{
	ProfilerWindow::ProfilerWindow()
	{
		m_Name = ICON_FA_STOPWATCH" Profiler###profiler";
		m_SimpleName = "Profiler";

		m_UpdateTimer = 0.0f;
		m_UpdateFrequency = 0.1f;
	}

	void ProfilerWindow::OnImGui()
	{
		if (Profiler::Instance()->IsEnabled())
		{
			m_UpdateTimer += Engine::Instance()->GetTimeStep()->GetElapsedMillis();
		}

		ImGui::Begin(m_Name.c_str());
		{
			auto profiler = Profiler::Instance();
			ImGui::Checkbox("Profiler Enabled", &profiler->IsEnabled());
			ImGui::InputFloat("Update Frequency", &m_UpdateFrequency);
			ImGui::Text("Report period duration : %f ms", m_Report.elaspedTime);
			ImGui::Text("Threads : %i", m_Report.workingThreads);
			ImGui::Text("Frames : %i", m_Report.elapsedFrames);

			if (profiler->IsEnabled())
			{
				if (m_UpdateTimer >= m_UpdateFrequency)
				{
					m_Report = profiler->GenerateReport();
					profiler->ClearHistory();
					m_UpdateTimer = 0.0f;
				}

				if (m_Report.actions.empty())
				{
					ImGui::Text("Collecting Data");
				}
				else
				{
					for (const auto& action : m_Report.actions)
					{
						ImVec4 colour;

						if (action.percentage <= 25.0f)
							colour = { 0.2f,0.8f,0.2f,1.0f };
						else if (action.percentage <= 50.0f)
							colour = { 0.2f,0.6f,0.6f,1.0f };
						else if (action.percentage <= 75.0f)
							colour = { 0.7f,0.2f,0.3f,1.0f };
						else
							colour = { 0.8f,0.2f,0.2f,1.0f };

						ImGui::TextColored(colour, "%s %f ms | %f ms per call | %f percent | %llu calls", action.name.c_str(), action.duration, action.duration / action.calls, action.percentage, action.calls);
					}
				}
			}
			else
			{
				ImGui::Text("Profiler Disabled");
			}
		}
		ImGui::End();
	}
}