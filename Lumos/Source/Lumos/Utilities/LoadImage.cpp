#include "Precompiled.h"
#include "LoadImage.h"

#include "Core/VFS.h"

#ifdef FREEIMAGE
#include <FreeImage.h>
#include <Utilities.h>
#else
#define STB_IMAGE_IMPLEMENTATION
#ifdef LUMOS_PLATFORM_LINUX
#define STBI_NO_SIMD
#endif
#include "stb_image.h"
#endif

namespace Lumos
{
    uint8_t* LoadImageFromFile(const char* filename, uint32_t* width, uint32_t* height, uint32_t* bits, bool* isHDR, bool flipY, bool srgb)
    {
        LUMOS_PROFILE_FUNCTION();
        std::string filePath = std::string(filename);
        std::string physicalPath;
        if(!VFS::Get()->ResolvePhysicalPath(filePath, physicalPath))
            return nullptr;

        filename = physicalPath.c_str();

        int texWidth = 0, texHeight = 0, texChannels = 0;
        stbi_uc* pixels = nullptr;
        int sizeOfChannel = 8;
        if(stbi_is_hdr(filename))
        {
            sizeOfChannel = 16;
            pixels = (uint8_t*)stbi_loadf(filename, &texWidth, &texHeight, &texChannels, 0);

            if(isHDR)
                *isHDR = true;
        }
        else
        {
            pixels = stbi_load(filename, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

            if(isHDR)
                *isHDR = false;
        }

        LUMOS_ASSERT(pixels, "Could not load image '{0}'!", filename);

        //TODO support different texChannels
        if(texChannels != 4)
            texChannels = 4;

        if(width)
            *width = texWidth;
        if(height)
            *height = texHeight;
        if(bits)
            *bits = texChannels * sizeOfChannel; // texChannels;	  //32 bits for 4 bytes r g b a

        const int32_t size = texWidth * texHeight * texChannels;
        uint8_t* result = new uint8_t[size];
        memcpy(result, pixels, size);

        stbi_image_free(pixels);
        return result;
    }

    uint8_t* LoadImageFromFile(const std::string& filename, uint32_t* width, uint32_t* height, uint32_t* bits, bool* isHDR, bool flipY, bool srgb)
    {
        return LoadImageFromFile(filename.c_str(), width, height, bits, isHDR, srgb, flipY);
    }
}
