#pragma once
#include "Core/Core.h"
#include <string>

namespace Lumos
{
    // 16-bit PCM WAV from interleaved float samples (-1..1). Physical or //VFS paths.
    LUMOS_EXPORT bool WriteWav16(const std::string& path, const float* interleaved, u32 frames, u32 channels, u32 sampleRate);
}
