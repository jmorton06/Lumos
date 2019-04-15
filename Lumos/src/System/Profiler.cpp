#include "LM.h"
#include "Profiler.h"
#include "Utilities/Timer.h"

#include <imgui/imgui.h>

namespace Lumos
{
	namespace system
	{
		namespace Profiler
		{
			bool initialized = false;
			bool ENABLED = false;

			struct ProfilerData
			{
				String name;
				String parent;
				bool secondary;
				float time;
				int count;
				Timer timer;
			};

			static std::unordered_map<String, ProfilerData> Data;
			static std::unordered_map<String, ProfilerData> previousData;


			void OnBegin()
			{
				if (!ENABLED)
					return;

				if (!initialized)
				{
					initialized = true;

					Data.reserve(100);
					previousData.reserve(100);
				}
			}

			void OnEnd()
			{
				if (!ENABLED || !initialized)
					return;

				previousData = Data;
				Data.clear();
			}

			void OnBeginRange(const String & name, bool secondary, const String & parentName)
			{
				if (!ENABLED || !initialized)
					return;

				std::unordered_map<String, ProfilerData>::iterator data = Data.find(name);

				if (data == Data.end())
				{
					ProfilerData newdata;
					newdata.name = name;
					newdata.secondary = secondary;
					newdata.count = 1;
					newdata.parent = parentName;
					newdata.time = 0.0f;
					newdata.timer.GetTimedMS();

					Data.emplace(name, newdata);
				}
				else
				{
					data->second.count++;
				}

			}

			void OnEndRange(const String & name, bool secondary, const String & parentName)
			{
				if (!ENABLED || !initialized)
					return;

				std::unordered_map<String, ProfilerData>::iterator data = Data.find(name);

				if (data != Data.end())
				{
					data->second.time += data->second.timer.GetTimedMS();
				}
				
			}

			void FindChild(const String& name)
			{
				for (auto& data : previousData)
				{
					if (data.second.secondary && data.second.parent == name)
					{
						ImGui::AlignTextToFramePadding();
						bool menuOpen = ImGui::TreeNode(data.first.c_str());
						ImGui::NextColumn();
						ImGui::PushItemWidth(-1);
						ImGui::AlignTextToFramePadding();
						ImGui::Text("%.3f", data.second.time);
						ImGui::PopItemWidth();
						ImGui::NextColumn();

						if (menuOpen)
						{
							FindChild(data.first);
							ImGui::TreePop();
						}
					}

				}
			};

			void OnImGUI()
			{
				if (!ENABLED || !initialized)
					return;

				ImGui::Begin("Profiler");
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
				ImGui::Columns(2);
				ImGui::Separator();

				for (auto& data : previousData)
				{
					if (!data.second.secondary)
					{
						ImGui::AlignTextToFramePadding();
						bool menuOpen = ImGui::TreeNode(data.first.c_str());
						ImGui::NextColumn();
						ImGui::PushItemWidth(-1);
						ImGui::AlignTextToFramePadding();
						ImGui::Text("%.3f", data.second.time);
						ImGui::PopItemWidth();
						ImGui::NextColumn();
						
						if(menuOpen)
						{
							FindChild(data.first);
							ImGui::TreePop();
						}
						
					}
				}
				ImGui::Columns(1);
				ImGui::Separator();
				ImGui::PopStyleVar();
				ImGui::End();
			}

			void SetEnabled(bool enabled)
			{
				ENABLED = enabled;
			}
		}
	}
}
