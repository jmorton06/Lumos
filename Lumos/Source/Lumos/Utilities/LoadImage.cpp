#include "Precompiled.h"
#include "LoadImage.h"

#include "Core/OS/FileSystem.h"
#include "Core/Asset/LImgReader.h"
#include "Utilities/StringUtilities.h"

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
        int texWidth = 0, texHeight = 0, texChannels = 0;
        stbi_uc* pixels   = nullptr;
        int sizeOfChannel = 8;

        ArenaTemp Scratch = ScratchBegin(0, 0);

        String8 filePath = Str8C((char*)filename);
        String8 physicalPath;

        if(!FileSystem::Get().ResolvePhysicalPath(Scratch.arena, filePath, &physicalPath))
        {
            ScratchEnd(Scratch);
            return nullptr;
        }

        // Fast path for .limg pre-decoded format
        {
            std::string ext = StringUtilities::GetFilePathExtension(std::string(filename));
            if(ext == "limg")
            {
                LImgReadResult imgResult = {};
                if(LImgReader::Read((const char*)physicalPath.str, imgResult))
                {
                    if(width)  *width  = imgResult.Width;
                    if(height) *height = imgResult.Height;
                    if(bits)   *bits   = imgResult.Bits;
                    if(isHDR)  *isHDR  = imgResult.IsHDR;
                    ScratchEnd(Scratch);
                    return imgResult.Pixels;
                }
            }
        }

        if(stbi_is_hdr((const char*)physicalPath.str))
        {
            sizeOfChannel = 32;
            pixels        = (uint8_t*)stbi_loadf((const char*)physicalPath.str, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

            if(isHDR)
                *isHDR = true;
        }
        else
        {
            pixels = stbi_load((const char*)physicalPath.str, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

            if(isHDR)
                *isHDR = false;
        }

        // Resize the image if it exceeds the maximum width or height
        // only callers that pass no isHDR get the size cap (the sprite editor, importer, cube maps want full size)
        const bool decodedHDR = sizeOfChannel == 32;
        if(pixels && !isHDR && !decodedHDR && s_MaxWidth > 0 && s_MaxHeight > 0 && ((uint32_t)texWidth > s_MaxWidth || (uint32_t)texHeight > s_MaxHeight))
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

            // stbi was asked for RGBA, whatever the file had
            uint8_t* resizedPixels = (stbi_uc*)malloc((size_t)texWidth * texHeight * 4);
            stbir_resize_uint8_linear(pixels, texWidthOld, texHeightOld, 0, resizedPixels, texWidth, texHeight, 0, STBIR_RGBA);

            free(pixels); // Free the original image
            pixels = resizedPixels;
        }

        if(!pixels)
        {
            LERROR("Could not load image '%s'!", filename);
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

            ScratchEnd(Scratch);
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
        ScratchEnd(Scratch);
        return result;
    }

    uint8_t* LoadImageFromFile(const std::string& filename, uint32_t* width, uint32_t* height, uint32_t* bits, bool* isHDR, bool flipY, bool srgb)
    {
        return LoadImageFromFile(filename.c_str(), width, height, bits, isHDR, srgb, flipY);
    }

    // Halves an RGBA8 image (2x2 average) until it fits maxDim; a pre-decoded .limg skips the stbi resize
    static void ShrinkToFit(ImageLoadDesc& desc, uint32_t maxDim)
    {
        if(desc.isHDR || maxDim == 0 || desc.outBits != 32)
            return;
        while(desc.outPixels && (desc.outWidth > maxDim || desc.outHeight > maxDim) && desc.outWidth >= 2 && desc.outHeight >= 2)
        {
            const uint32_t w = desc.outWidth, h = desc.outHeight, nw = w / 2, nh = h / 2;
            const uint8_t* src = (const uint8_t*)desc.outPixels;
            uint8_t* dst       = new uint8_t[(size_t)nw * nh * 4];
            for(uint32_t y = 0; y < nh; y++)
            {
                const uint8_t* r0 = src + (size_t)(2 * y) * w * 4;
                const uint8_t* r1 = r0 + (size_t)w * 4;
                uint8_t* out      = dst + (size_t)y * nw * 4;
                for(uint32_t x = 0; x < nw; x++)
                    for(int c = 0; c < 4; c++)
                        out[x * 4 + c] = (uint8_t)((r0[x * 8 + c] + r0[x * 8 + 4 + c] + r1[x * 8 + c] + r1[x * 8 + 4 + c] + 2) >> 2);
            }
            delete[] (uint8_t*)desc.outPixels;
            desc.outPixels = dst;
            desc.outWidth  = nw;
            desc.outHeight = nh;
        }
    }

    bool LoadImageFromFile(ImageLoadDesc& desc)
    {
        LUMOS_PROFILE_FUNCTION();
        stbi_uc* pixels = nullptr;
        int texWidth = 0, texHeight = 0, texChannels = 0;
        int sizeOfChannel = 8;

        ArenaTemp Scratch = ScratchBegin(0, 0);

        String8 filePath = Str8C((char*)desc.filePath);

        String8 physicalPath;
        const bool havePhysical = FileSystem::Get().ResolvePhysicalPath(Scratch.arena, filePath, &physicalPath)
                                  && FileSystem::FileExists(physicalPath);

        if(havePhysical)
        {
            desc.filePath   = (const char*)physicalPath.str;
            std::string ext = StringUtilities::GetFilePathExtension(std::string(desc.filePath));
            if(ext == "limg")
            {
                LImgReadResult imgResult = {};
                if(LImgReader::Read(desc.filePath, imgResult))
                {
                    desc.outWidth  = imgResult.Width;
                    desc.outHeight = imgResult.Height;
                    desc.outBits   = imgResult.Bits;
                    desc.isHDR     = imgResult.IsHDR;
                    desc.outPixels = imgResult.Pixels;
                    ShrinkToFit(desc, desc.maxWidth);
                    ScratchEnd(Scratch);
                    return true;
                }
                // Fall through to stbi if limg read fails
            }
        }

        if(havePhysical)
        {
            if(stbi_is_hdr(desc.filePath))
            {
                sizeOfChannel = 32;
                pixels        = (uint8_t*)stbi_loadf(desc.filePath, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
                desc.isHDR    = true;
            }
            else
            {
                pixels     = stbi_load(desc.filePath, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
                desc.isHDR = false;
            }
        }
        else
        {
            // textures are bigger than the scratch arena: read into one sized for the file
            const int64_t encodedSz = FileSystem::Get().GetFileSizeVFS(filePath);
            if(encodedSz <= 0)
            {
                ScratchEnd(Scratch);
                return false;
            }
            struct ArenaGuard
            {
                Arena* A;
                ~ArenaGuard() { ArenaRelease(A); }
            } dataArena = { ArenaAlloc((u64)encodedSz + 4096) };
            u8* encoded = FileSystem::Get().ReadFileVFS(dataArena.A, filePath);
            if(!encoded)
            {
                ScratchEnd(Scratch);
                return false;
            }
            // pre-decoded .limg inside a pack
            if(StringUtilities::GetFilePathExtension(std::string(desc.filePath)) == "limg")
            {
                LImgReadResult imgResult = {};
                if(LImgReader::ReadMemory(encoded, (u64)encodedSz, imgResult))
                {
                    desc.outWidth  = imgResult.Width;
                    desc.outHeight = imgResult.Height;
                    desc.outBits   = imgResult.Bits;
                    desc.isHDR     = imgResult.IsHDR;
                    desc.outPixels = imgResult.Pixels;
                    ShrinkToFit(desc, desc.maxWidth);
                    ScratchEnd(Scratch);
                    return true;
                }
            }
            if(stbi_is_hdr_from_memory(encoded, (int)encodedSz))
            {
                sizeOfChannel = 32;
                pixels        = (uint8_t*)stbi_loadf_from_memory(encoded, (int)encodedSz, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
                desc.isHDR    = true;
            }
            else
            {
                pixels     = stbi_load_from_memory(encoded, (int)encodedSz, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
                desc.isHDR = false;
            }
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

        if(!pixels)
        {
            LERROR("Could not load image '%s'!", desc.filePath);
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
            ScratchEnd(Scratch);
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

        ScratchEnd(Scratch);
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

    uint8_t* LoadImageFromMemory(const uint8_t* data, size_t size, uint32_t* width, uint32_t* height, uint32_t maxSize)
    {
        if(!data || size == 0)
            return nullptr;

        int w = 0, h = 0, channels = 0;
        stbi_uc* pixels = stbi_load_from_memory(data, (int)size, &w, &h, &channels, STBI_rgb_alpha);
        if(!pixels)
            return nullptr;

        if(maxSize > 0 && ((uint32_t)w > maxSize || (uint32_t)h > maxSize))
        {
            float scale  = (float)maxSize / (float)(w > h ? w : h);
            int nw       = w * scale < 1.0f ? 1 : (int)(w * scale);
            int nh       = h * scale < 1.0f ? 1 : (int)(h * scale);
            stbi_uc* out = (stbi_uc*)malloc((size_t)nw * nh * 4);
            if(out && stbir_resize_uint8_srgb(pixels, w, h, 0, out, nw, nh, 0, STBIR_RGBA))
            {
                stbi_image_free(pixels);
                pixels = out;
                w      = nw;
                h      = nh;
            }
            else
                free(out);
        }

        if(width)
            *width = (uint32_t)w;
        if(height)
            *height = (uint32_t)h;
        return pixels;
    }

    bool ImageAccentColour(const uint8_t* data, size_t size, float outRGB[3])
    {
        uint32_t w = 0, h = 0;
        uint8_t* px = LoadImageFromMemory(data, size, &w, &h, 48);
        if(!px)
            return false;

        // hue histogram weighted by how vivid each pixel is
        constexpr int Bins = 24;
        float weight[Bins] = {};
        float sum[Bins][3] = {};
        float total = 0.0f;
        for(uint32_t i = 0; i < w * h; i++)
        {
            float r = px[i * 4] / 255.0f, g = px[i * 4 + 1] / 255.0f, b = px[i * 4 + 2] / 255.0f;
            float mx = r > g ? (r > b ? r : b) : (g > b ? g : b);
            float mn = r < g ? (r < b ? r : b) : (g < b ? g : b);
            float d  = mx - mn;
            if(mx < 0.12f || d < 0.08f)
                continue;
            float s = d / mx;
            float hue;
            if(mx == r)
                hue = (g - b) / d;
            else if(mx == g)
                hue = 2.0f + (b - r) / d;
            else
                hue = 4.0f + (r - g) / d;
            hue = hue / 6.0f;
            if(hue < 0.0f)
                hue += 1.0f;
            int bin   = (int)(hue * Bins) % Bins;
            float wgt = s * s * mx;
            weight[bin] += wgt;
            sum[bin][0] += r * wgt;
            sum[bin][1] += g * wgt;
            sum[bin][2] += b * wgt;
            total += wgt;
        }
        FreeImageMemory(px);

        // neighbouring bins count too, so a hue split across a boundary still wins
        int best      = -1;
        float bestW   = 0.0f;
        for(int i = 0; i < Bins; i++)
        {
            float wgt = weight[i] + 0.5f * (weight[(i + 1) % Bins] + weight[(i + Bins - 1) % Bins]);
            if(wgt > bestW)
            {
                bestW = wgt;
                best  = i;
            }
        }
        if(best < 0 || total < (float)(w * h) * 0.01f || weight[best] <= 0.0f)
            return false;
        for(int c = 0; c < 3; c++)
            outRGB[c] = sum[best][c] / weight[best];
        return true;
    }

    void FreeImageMemory(uint8_t* pixels)
    {
        // stbi uses malloc/free by default
        free(pixels);
    }
}
