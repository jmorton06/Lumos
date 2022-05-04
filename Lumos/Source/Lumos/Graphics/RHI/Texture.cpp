#include "Precompiled.h"
#include "Texture.h"

#include "Utilities/LoadImage.h"

namespace Lumos
{
    namespace Graphics
    {
        Texture2D* (*Texture2D::CreateFunc)() = nullptr;
        Texture2D* (*Texture2D::CreateFromSourceFunc)(uint32_t, uint32_t, void*, TextureParameters, TextureLoadOptions) = nullptr;
        Texture2D* (*Texture2D::CreateFromFileFunc)(const std::string&, const std::string&, TextureParameters, TextureLoadOptions) = nullptr;

        TextureDepth* (*TextureDepth::CreateFunc)(uint32_t, uint32_t) = nullptr;
        TextureDepthArray* (*TextureDepthArray::CreateFunc)(uint32_t, uint32_t, uint32_t) = nullptr;

        TextureCube* (*TextureCube::CreateFunc)(uint32_t, void*, bool) = nullptr;
        TextureCube* (*TextureCube::CreateFromFileFunc)(const std::string&) = nullptr;
        TextureCube* (*TextureCube::CreateFromFilesFunc)(const std::string*) = nullptr;
        TextureCube* (*TextureCube::CreateFromVCrossFunc)(const std::string*, uint32_t, TextureParameters, TextureLoadOptions) = nullptr;

        uint8_t Texture::GetStrideFromFormat(const Format format)
        {
            switch(format)
            {
            case Format::R8_Unorm:
            case Format::D16_Unorm:
                return 1;
            case Format::R8G8_Unorm:
                return 2;
            case Format::R8G8B8_Unorm:
            case Format::R16G16B16_Float:
            case Format::R32G32B32_Float:
                return 3;
            case Format::R8G8B8A8_Unorm:
            case Format::R16G16B16A16_Float:
            case Format::R32G32B32A32_Float:
                return 4;
            default:
                return 0;
            }
        }

        Format Texture::BitsToFormat(uint32_t bits)
        {
            switch(bits)
            {
            case 8:
                return Format::R8_Unorm;
            case 16:
                return Format::R8G8_Unorm;
            case 24:
                return Format::R8G8B8_Unorm;
            case 32:
                return Format::R8G8B8A8_Unorm;
            case 48:
                return Format::R16G16B16_Float;
            case 64:
                return Format::R16G16B16A16_Float;
            case 96:
                return Format::R32G32B32_Float;
            case 128:
                return Format::R32G32B32A32_Float;
            default:
                LUMOS_ASSERT(false, "[Texture] Unsupported image bit-depth! ({0})", bits);
                return Format::R8G8B8A8_Unorm;
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
                LUMOS_ASSERT(false, "[Texture] Unsupported image bit-depth! ({0})", bits);
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

        Texture2D* Texture2D::Create()
        {
            LUMOS_ASSERT(CreateFunc, "No Texture2D Create Function");

            return CreateFunc();
        }

        Texture2D* Texture2D::CreateFromSource(uint32_t width, uint32_t height, void* data, TextureParameters parameters, TextureLoadOptions loadOptions)
        {
            LUMOS_ASSERT(CreateFromSourceFunc, "No Texture2D Create Function");

            return CreateFromSourceFunc(width, height, data, parameters, loadOptions);
        }

        Texture2D* Texture2D::CreateFromFile(const std::string& name, const std::string& filepath, TextureParameters parameters, TextureLoadOptions loadOptions)
        {
            LUMOS_ASSERT(CreateFromFileFunc, "No Texture2D Create Function");

            return CreateFromFileFunc(name, filepath, parameters, loadOptions);
        }

        TextureCube* TextureCube::Create(uint32_t size, void* data, bool hdr)
        {
            LUMOS_ASSERT(CreateFunc, "No TextureCube Create Function");

            return CreateFunc(size, data, hdr);
        }

        TextureCube* TextureCube::CreateFromFile(const std::string& filepath)
        {
            LUMOS_ASSERT(CreateFromFileFunc, "No TextureCube Create Function");

            return CreateFromFileFunc(filepath);
        }

        TextureCube* TextureCube::CreateFromFiles(const std::string* files)
        {
            LUMOS_ASSERT(CreateFromFilesFunc, "No TextureCube Create Function");

            return CreateFromFilesFunc(files);
        }

        TextureCube* TextureCube::CreateFromVCross(const std::string* files, uint32_t mips, TextureParameters params, TextureLoadOptions loadOptions)
        {
            LUMOS_ASSERT(CreateFromVCrossFunc, "No TextureCube Create Function");

            return CreateFromVCrossFunc(files, mips, params, loadOptions);
        }

        TextureDepth* TextureDepth::Create(uint32_t width, uint32_t height)
        {
            LUMOS_ASSERT(CreateFunc, "No TextureDepth Create Function");

            return CreateFunc(width, height);
        }

        TextureDepthArray* TextureDepthArray::Create(uint32_t width, uint32_t height, uint32_t count)
        {
            LUMOS_ASSERT(CreateFunc, "No TextureDepthArray Create Function");

            return CreateFunc(width, height, count);
        }
    }
}
