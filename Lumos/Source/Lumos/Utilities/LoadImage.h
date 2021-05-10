#pragma once

namespace Lumos
{
    LUMOS_EXPORT uint8_t* LoadImageFromFile(const char* filename, uint32_t* width = nullptr, uint32_t* height = nullptr, uint32_t* bits = nullptr, bool* isHDR = nullptr, bool flipY = false, bool srgb = true);
    LUMOS_EXPORT uint8_t* LoadImageFromFile(const std::string& filename, uint32_t* width = nullptr, uint32_t* height = nullptr, uint32_t* bits = nullptr, bool* isHDR = nullptr, bool flipY = false, bool srgb = true);
}
