#include "Precompiled.h"
#include "Texture.h"

#include "Utilities/LoadImage.h"

namespace Lumos
{
    namespace Graphics
    {
        Texture2D* (*Texture2D::CreateFunc)(TextureDesc, uint32_t, uint32_t)                                                 = nullptr;
        Texture2D* (*Texture2D::CreateFromSourceFunc)(uint32_t, uint32_t, void*, TextureDesc, TextureLoadOptions)            = nullptr;
        Texture2D* (*Texture2D::CreateFromFileFunc)(const std::string&, const std::string&, TextureDesc, TextureLoadOptions) = nullptr;

        TextureDepth* (*TextureDepth::CreateFunc)(uint32_t, uint32_t, RHIFormat, uint8_t)            = nullptr;
        TextureDepthArray* (*TextureDepthArray::CreateFunc)(uint32_t, uint32_t, uint32_t, RHIFormat) = nullptr;

        TextureCube* (*TextureCube::CreateFunc)(uint32_t, void*, bool)                                                   = nullptr;
        TextureCube* (*TextureCube::CreateFromFileFunc)(const std::string&)                                              = nullptr;
        TextureCube* (*TextureCube::CreateFromFilesFunc)(const std::string*)                                             = nullptr;
        TextureCube* (*TextureCube::CreateFromVCrossFunc)(const std::string*, uint32_t, TextureDesc, TextureLoadOptions) = nullptr;

        uint8_t Texture::GetStrideFromFormat(const RHIFormat format)
        {
            switch(format)
            {
            case RHIFormat::R8_Unorm:
            case RHIFormat::D16_Unorm:
                return 1;
            case RHIFormat::R8G8_Unorm:
                return 2;
            case RHIFormat::R8G8B8_Unorm:
            case RHIFormat::R16G16B16_Float:
            case RHIFormat::R32G32B32_Float:
                return 3;
            case RHIFormat::R8G8B8A8_Unorm:
            case RHIFormat::R16G16B16A16_Float:
            case RHIFormat::R32G32B32A32_Float:
                return 4;
            default:
                return 0;
            }
        }

        uint32_t Texture::GetBitsFromFormat(const RHIFormat format)
        {
            switch(format)
            {
            case RHIFormat::R8_Unorm:
                return 8;
            case RHIFormat::D16_Unorm:
                return 16;
            case RHIFormat::R8G8_Unorm:
                return 16;
            case RHIFormat::R8G8B8_Unorm:
                return 24;
            case RHIFormat::R16G16B16_Float:
                return 48;
            case RHIFormat::R32G32B32_Float:
                return 96;
            case RHIFormat::R8G8B8A8_Unorm:
                return 32;
            case RHIFormat::R16G16B16A16_Float:
                return 64;
            case RHIFormat::R32G32B32A32_Float:
                return 128;
            default:
                return 32;
            }
        }

        RHIFormat Texture::BitsToFormat(uint32_t bits)
        {
            switch(bits)
            {
            case 8:
                return RHIFormat::R8_Unorm;
            case 16:
                return RHIFormat::R8G8_Unorm;
            case 24:
                return RHIFormat::R8G8B8_Unorm;
            case 32:
                return RHIFormat::R8G8B8A8_Unorm;
            case 48:
                return RHIFormat::R16G16B16_Float;
            case 64:
                return RHIFormat::R16G16B16A16_Float;
            case 96:
                return RHIFormat::R32G32B32_Float;
            case 128:
                return RHIFormat::R32G32B32A32_Float;
            default:
                ASSERT(false, "[Texture] Unsupported image bit-depth! (%i)", bits);

                return RHIFormat::R8G8B8A8_Unorm;
            }
        }

        uint32_t Texture::BitsToChannelCount(uint32_t bits)
        {
            switch(bits)
            {
            case 8:
                return 1;
            case 16:
                return 2;
            case 24:
                return 3;
            case 32:
                return 4;
            case 48:
                return 3;
            case 64:
                return 4;
            case 96:
                return 3;
            case 128:
                return 4;
            default:
                ASSERT(false, "[Texture] Unsupported image bit-depth! (%i)", bits);
                return 4;
            }
        }

        uint32_t Texture::CalculateMipMapCount(uint32_t width, uint32_t height)
        {
            uint32_t levels = 1;
            while((width | height) >> levels)
                levels++;

            return levels;
        }

        Texture2D* Texture2D::Create(TextureDesc parameters, uint32_t width, uint32_t height)
        {
            ASSERT(CreateFunc, "No Texture2D Create Function");

            return CreateFunc(parameters, width, height);
        }

        Texture2D* Texture2D::CreateFromSource(uint32_t width, uint32_t height, void* data, TextureDesc parameters, TextureLoadOptions loadOptions)
        {
            ASSERT(CreateFromSourceFunc, "No Texture2D Create Function");

            return CreateFromSourceFunc(width, height, data, parameters, loadOptions);
        }

        Texture2D* Texture2D::CreateFromFile(const std::string& name, const std::string& filepath, TextureDesc parameters, TextureLoadOptions loadOptions)
        {
            ASSERT(CreateFromFileFunc, "No Texture2D Create Function");

            return CreateFromFileFunc(name, filepath, parameters, loadOptions);
        }

        TextureCube* TextureCube::Create(uint32_t size, void* data, bool hdr)
        {
            ASSERT(CreateFunc, "No TextureCube Create Function");

            return CreateFunc(size, data, hdr);
        }

        TextureCube* TextureCube::CreateFromFile(const std::string& filepath)
        {
            ASSERT(CreateFromFileFunc, "No TextureCube Create Function");

            return CreateFromFileFunc(filepath);
        }

        TextureCube* TextureCube::CreateFromFiles(const std::string* files)
        {
            ASSERT(CreateFromFilesFunc, "No TextureCube Create Function");

            return CreateFromFilesFunc(files);
        }

        TextureCube* TextureCube::CreateFromVCross(const std::string* files, uint32_t mips, TextureDesc params, TextureLoadOptions loadOptions)
        {
            ASSERT(CreateFromVCrossFunc, "No TextureCube Create Function");

            return CreateFromVCrossFunc(files, mips, params, loadOptions);
        }

        TextureDepth* TextureDepth::Create(uint32_t width, uint32_t height, RHIFormat format, uint8_t samples)
        {
            ASSERT(CreateFunc, "No TextureDepth Create Function");

            return CreateFunc(width, height, format, samples);
        }

        TextureDepthArray* TextureDepthArray::Create(uint32_t width, uint32_t height, uint32_t count, RHIFormat format)
        {
            ASSERT(CreateFunc, "No TextureDepthArray Create Function");

            return CreateFunc(width, height, count, format);
        }
    }
}
