#pragma once
#include "LM.h"

namespace Lumos
{
	LUMOS_EXPORT byte* LoadImageFromFile(const char* filename, u32* width = nullptr, u32* height = nullptr, u32* bits = nullptr, bool flipY = false);
	LUMOS_EXPORT byte* LoadImageFromFile(const String& filename, u32* width = nullptr, u32* height = nullptr, u32* bits = nullptr, bool flipY = false);
}