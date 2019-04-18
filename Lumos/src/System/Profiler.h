#pragma once
#include "LM.h"

namespace Lumos
{
	namespace system
	{
		namespace Profiler
		{
			void OnInit();
			void OnBegin();
			void OnEnd();

			void OnBeginRange(const String& name, bool secondary = false, const String& parentName = "", bool startUp = false);
			void OnEndRange(const String& name, bool secondary = false, const String& parentName = "", bool startUp = false);

			void OnImGUI();

			void SetEnabled(bool enabled);
		};
	}
}