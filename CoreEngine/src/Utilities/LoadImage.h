#pragma once
#include "JM.h"

namespace jm
{
	JM_EXPORT byte* LoadImageFromFile(const char* filename, uint* width = nullptr, uint* height = nullptr, uint* bits = nullptr, bool flipY = false);
	JM_EXPORT byte* LoadImageFromFile(const String& filename, uint* width = nullptr, uint* height = nullptr, uint* bits = nullptr, bool flipY = false);
}