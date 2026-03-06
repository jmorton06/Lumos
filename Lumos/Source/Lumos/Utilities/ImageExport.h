#pragma once
#include <string>
#include <cstdint>

namespace Lumos
{
    class ImageExport
    {
    public:
        // Save RGBA pixel data to PNG
        static bool SavePNG(const std::string& path, uint32_t width, uint32_t height, const uint8_t* data);

        // Save RGBA pixel data to JPG (quality 1-100)
        static bool SaveJPG(const std::string& path, uint32_t width, uint32_t height, const uint8_t* data, int quality = 90);

        // Save RGBA pixel data to BMP
        static bool SaveBMP(const std::string& path, uint32_t width, uint32_t height, const uint8_t* data);
    };
}
