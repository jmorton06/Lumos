#pragma once

namespace Lumos
{
    struct ImageLoadDesc
    {
        const char* filePath;
        uint32_t outWidth;
        uint32_t outHeight;
        uint32_t outBits;
        bool isHDR;
        bool flipY         = false;
        bool srgb          = true;
        uint32_t maxWidth  = 2048;
        uint32_t maxHeight = 2048;
        uint8_t* outPixels;
    };

    LUMOS_EXPORT uint8_t* LoadImageFromFile(const char* filename, uint32_t* width = nullptr, uint32_t* height = nullptr, uint32_t* bits = nullptr, bool* isHDR = nullptr, bool flipY = false, bool srgb = true);
    LUMOS_EXPORT uint8_t* LoadImageFromFile(const std::string& filename, uint32_t* width = nullptr, uint32_t* height = nullptr, uint32_t* bits = nullptr, bool* isHDR = nullptr, bool flipY = false, bool srgb = true);

    LUMOS_EXPORT bool LoadImageFromFile(ImageLoadDesc& desc);

    // Decodes an encoded image (png/jpg/...) held in memory to RGBA8, downscaled to fit maxSize
    // (0 = no limit). Free the result with FreeImageMemory.
    LUMOS_EXPORT uint8_t* LoadImageFromMemory(const uint8_t* data, size_t size, uint32_t* width, uint32_t* height, uint32_t maxSize = 0);
    LUMOS_EXPORT void FreeImageMemory(uint8_t* pixels);

    // Most prominent saturated colour of an encoded image (rgb 0-1). False for greyscale-ish images.
    LUMOS_EXPORT bool ImageAccentColour(const uint8_t* data, size_t size, float outRGB[3]);

    LUMOS_EXPORT void SetMaxImageDimensions(uint32_t width, uint32_t height);

    LUMOS_EXPORT void GetMaxImageDimensions(uint32_t& width, uint32_t& height);
}
