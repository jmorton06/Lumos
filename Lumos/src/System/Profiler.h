#pragma once
#include "LM.h"

#define LUMOS_PROFILER

#ifdef LUMOS_PROFILER
#define LUMOS_PROFILE(x) x
#else
#define LUMOS_PROFILE(x)
#endif

namespace Lumos
{
	namespace System
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