#include "Precompiled.h"
#include "ImageExport.h"
#include <stb/stb_image_write.h>

namespace Lumos
{
    bool ImageExport::SavePNG(const std::string& path, uint32_t width, uint32_t height, const uint8_t* data)
    {
        return stbi_write_png(path.c_str(), (int)width, (int)height, 4, data, (int)(width * 4)) != 0;
    }

    bool ImageExport::SaveJPG(const std::string& path, uint32_t width, uint32_t height, const uint8_t* data, int quality)
    {
        return stbi_write_jpg(path.c_str(), (int)width, (int)height, 4, data, quality) != 0;
    }

    bool ImageExport::SaveBMP(const std::string& path, uint32_t width, uint32_t height, const uint8_t* data)
    {
        return stbi_write_bmp(path.c_str(), (int)width, (int)height, 4, data) != 0;
    }
}
