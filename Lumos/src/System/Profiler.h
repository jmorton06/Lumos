#pragma once
#include "LM.h"

namespace Lumos
{
	namespace system
	{
		namespace Profiler
		{
			void OnBegin();
			void OnEnd();

			void OnBeginRange(const String& name, bool secondary = false, const String& parentName = "");
			void OnEndRange(const String& name, bool secondary = false, const String& parentName = "");

			void OnImGUI();

			void SetEnabled(bool enabled);
		};
	}
}