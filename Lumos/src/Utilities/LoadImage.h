#pragma once
#include "LM.h"

namespace lumos
{
	LUMOS_EXPORT byte* LoadImageFromFile(const char* filename, uint* width = nullptr, uint* height = nullptr, uint* bits = nullptr, bool flipY = false);
	LUMOS_EXPORT byte* LoadImageFromFile(const String& filename, uint* width = nullptr, uint* height = nullptr, uint* bits = nullptr, bool flipY = false);
}