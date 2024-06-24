#ifndef LUMOS_PLATFORM_MACOS
#include "Precompiled.h"
#endif
#include "LoadImage.h"

#include "Core/OS/FileSystem.h"

#ifdef FREEIMAGE
#include <FreeImage.h>
#include <Utilities.h>
#else
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#ifdef LUMOS_PLATFORM_LINUX
#define STBI_NO_SIMD
#endif
#include "stb_image.h"
#include "stb_image_resize2.h"
#endif

namespace Lumos
{

    static uint32_t s_MaxWidth  = 0;
    static uint32_t s_MaxHeight = 0;

    uint8_t* LoadImageFromFile(const char* filename, uint32_t* width, uint32_t* height, uint32_t* bits, bool* isHDR, bool flipY, bool srgb)
    {
        LUMOS_PROFILE_FUNCTION();
        std::string filePath = std::string(filename);
        std::string physicalPath;
        if(!FileSystem::Get().ResolvePhysicalPath(filePath, physicalPath))
            return nullptr;

        filename = physicalPath.c_str();

        int texWidth = 0, texHeight = 0, texChannels = 0;
        stbi_uc* pixels   = nullptr;
        int sizeOfChannel = 8;
        if(stbi_is_hdr(filename))
        {
            sizeOfChannel = 32;
            pixels        = (uint8_t*)stbi_loadf(filename, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

            if(isHDR)
                *isHDR = true;
        }
        else
        {
            pixels = stbi_load(filename, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

            if(isHDR)
                *isHDR = false;
        }

        // Resize the image if it exceeds the maximum width or height
        if(!isHDR && s_MaxWidth > 0 && s_MaxHeight > 0 && ((uint32_t)texWidth > s_MaxWidth || (uint32_t)texHeight > s_MaxHeight))
        {
            uint32_t texWidthOld = texWidth, texHeightOld = texHeight;
            float aspectRatio = static_cast<float>(texWidth) / static_cast<float>(texHeight);
            if((uint32_t)texWidth > s_MaxWidth)
            {
                texWidth  = s_MaxWidth;
                texHeight = static_cast<uint32_t>(s_MaxWidth / aspectRatio);
            }
            if((uint32_t)texHeight > s_MaxHeight)
            {
                texHeight = s_MaxHeight;
                texWidth  = static_cast<uint32_t>(s_MaxHeight * aspectRatio);
            }

            // Resize the image using stbir
            int resizedChannels    = texChannels;
            uint8_t* resizedPixels = (stbi_uc*)malloc(texWidth * texHeight * resizedChannels);

            if(isHDR)
            {
                stbir_resize_float_linear((float*)pixels, texWidthOld, texHeightOld, 0, (float*)resizedPixels, texWidth, texHeight, 0, STBIR_RGBA);
            }
            else
            {
                stbir_resize_uint8_linear(pixels, texWidthOld, texHeightOld, 0, resizedPixels, texWidth, texHeight, 0, STBIR_RGBA);
            }

            free(pixels); // Free the original image
            pixels = resizedPixels;
        }

        if(!pixels)
        {
            LUMOS_LOG_ERROR("Could not load image '{0}'!", filename);
            // Return magenta checkerboad image

            texChannels = 4;

            if(width)
                *width = 2;
            if(height)
                *height = 2;
            if(bits)
                *bits = texChannels * sizeOfChannel;

            const int32_t size = (*width) * (*height) * texChannels;
            uint8_t* data      = new uint8_t[size];

            uint8_t datatwo[16] = {
                255, 0, 255, 255,
                0, 0, 0, 255,
                0, 0, 0, 255,
                255, 0, 255, 255
            };

            memcpy(data, datatwo, size);

            return data;
        }

        // TODO support different texChannels
        if(texChannels != 4)
            texChannels = 4;

        if(width)
            *width = texWidth;
        if(height)
            *height = texHeight;
        if(bits)
            *bits = texChannels * sizeOfChannel; // texChannels;	  //32 bits for 4 bytes r g b a

        const uint64_t size = uint64_t(texWidth) * uint64_t(texHeight) * uint64_t(texChannels) * uint64_t(sizeOfChannel / 8U);
        uint8_t* result     = new uint8_t[size];
        memcpy(result, pixels, size);

        stbi_image_free(pixels);
        return result;
    }

    uint8_t* LoadImageFromFile(const std::string& filename, uint32_t* width, uint32_t* height, uint32_t* bits, bool* isHDR, bool flipY, bool srgb)
    {
        return LoadImageFromFile(filename.c_str(), width, height, bits, isHDR, srgb, flipY);
    }

    bool LoadImageFromFile(ImageLoadDesc& desc)
    {
        LUMOS_PROFILE_FUNCTION();
        std::string filePath = std::string(desc.filePath);
        std::string physicalPath;
        stbi_uc* pixels = nullptr;
        int texWidth = 0, texHeight = 0, texChannels = 0;

        int sizeOfChannel = 8;
        if(FileSystem::Get().ResolvePhysicalPath(filePath, physicalPath))
        {
            desc.filePath = physicalPath.c_str();

            if(stbi_is_hdr(desc.filePath))
            {
                sizeOfChannel = 32;
                pixels        = (uint8_t*)stbi_loadf(desc.filePath, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

                desc.isHDR = true;
            }
            else
            {
                pixels = stbi_load(desc.filePath, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

                desc.isHDR = false;
            }

            // Resize the image if it exceeds the maximum width or height
            if(!desc.isHDR && desc.maxWidth > 0 && desc.maxHeight > 0 && ((uint32_t)texWidth > desc.maxWidth || (uint32_t)texHeight > desc.maxHeight))
            {
                uint32_t texWidthOld = texWidth, texHeightOld = texHeight;
                float aspectRatio = static_cast<float>(texWidth) / static_cast<float>(texHeight);
                if((uint32_t)texWidth > desc.maxWidth)
                {
                    texWidth  = desc.maxWidth;
                    texHeight = static_cast<uint32_t>(desc.maxWidth / aspectRatio);
                }
                if((uint32_t)texHeight > desc.maxHeight)
                {
                    texHeight = desc.maxHeight;
                    texWidth  = static_cast<uint32_t>(desc.maxHeight * aspectRatio);
                }

                if(texChannels != 4)
                    texChannels = 4;

                // Resize the image using stbir
                int resizedChannels    = texChannels;
                uint8_t* resizedPixels = (stbi_uc*)malloc(texWidth * texHeight * resizedChannels);

                if(desc.isHDR)
                {
                    stbir_resize_float_linear((float*)pixels, texWidthOld, texHeightOld, 0, (float*)resizedPixels, texWidth, texHeight, 0, STBIR_RGBA);
                }
                else
                {
                    stbir_resize_uint8_linear(pixels, texWidthOld, texHeightOld, 0, resizedPixels, texWidth, texHeight, 0, texChannels == 4 ? STBIR_RGBA : STBIR_RGB);
                }

                stbi_image_free(pixels); // Free the original image
                pixels = resizedPixels;
            }
        }

        if(!pixels)
        {
            LUMOS_LOG_ERROR("Could not load image '{0}'!", desc.filePath);
            // Return magenta checkerboard image

            texChannels = 4;

            desc.outWidth  = 2;
            desc.outHeight = 2;
            desc.outBits   = texChannels * sizeOfChannel;

            const int32_t size = desc.outWidth * desc.outHeight * texChannels;
            uint8_t* data      = new uint8_t[size];

            uint8_t datatwo[16] = {
                255, 0, 255, 255,
                0, 0, 0, 255,
                0, 0, 0, 255,
                255, 0, 255, 255
            };

            memcpy(data, datatwo, size);

            desc.outPixels = data;
            return false;
        }

        // TODO support different texChannels
        if(texChannels != 4)
            texChannels = 4;

        desc.outWidth  = texWidth;
        desc.outHeight = texHeight;
        desc.outBits   = texChannels * sizeOfChannel; // texChannels;	  //32 bits for 4 bytes r g b a

        const uint64_t size = uint64_t(texWidth) * uint64_t(texHeight) * uint64_t(texChannels) * uint64_t(sizeOfChannel / 8U);
        uint8_t* result     = new uint8_t[size];
        memcpy(result, pixels, size);

        stbi_image_free(pixels);
        desc.outPixels = result;
        return true;
    }

    void SetMaxImageDimensions(uint32_t width, uint32_t height)
    {
        s_MaxWidth  = width;
        s_MaxHeight = height;
    }

    void GetMaxImageDimensions(uint32_t& width, uint32_t& height)
    {
        width  = s_MaxWidth;
        height = s_MaxHeight;
    }
}
