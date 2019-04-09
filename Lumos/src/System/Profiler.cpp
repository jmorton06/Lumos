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

				auto* data = &Data[name];

				if (!data)
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
					data->count++;
				}

			}

			void OnEndRange(const String & name, bool secondary, const String & parentName)
			{
				if (!ENABLED || !initialized)
					return;

				auto* data = &Data[name];

				if (data)
				{
					data->time += data->timer.GetTimedMS();
				}
			}

			void OnImGUI()
			{
				if (!ENABLED || !initialized)
					return;

				ImGui::Begin("Profiler");

				for (auto& data : previousData)
				{
					String s = " %.3f ms";
					ImGui::Text((data.first + s).c_str(), data.second.time);
				}

				ImGui::End();
			}

			void SetEnabled(bool enabled)
			{
				ENABLED = enabled;
			}
		}
	}
}
