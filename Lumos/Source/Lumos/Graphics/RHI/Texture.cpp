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

        TextureCube* (*TextureCube::CreateFunc)(uint32_t) = nullptr;
        TextureCube* (*TextureCube::CreateFromFileFunc)(const std::string&) = nullptr;
        TextureCube* (*TextureCube::CreateFromFilesFunc)(const std::string*) = nullptr;
        TextureCube* (*TextureCube::CreateFromVCrossFunc)(const std::string*, uint32_t, TextureParameters, TextureLoadOptions, InputFormat) = nullptr;

        uint8_t Texture::GetStrideFromFormat(const TextureFormat format)
        {
            switch(format)
            {
            case TextureFormat::RGB:
                return 3;
            case TextureFormat::RGBA:
                return 4;
            case TextureFormat::R8:
                return 1;
            case TextureFormat::RG8:
                return 2;
            case TextureFormat::RGB8:
                return 3;
            case TextureFormat::RGBA8:
                return 4;
            default:
                return 0;
            }
        }

        TextureFormat Texture::BitsToTextureFormat(uint32_t bits)
        {
            switch(bits)
            {
            case 8:
                return TextureFormat::R8;
            case 16:
                return TextureFormat::RG8;
            case 24:
                return TextureFormat::RGB8;
            case 32:
                return TextureFormat::RGBA8;
            case 48:
                return TextureFormat::RGB16;
            case 64:
                return TextureFormat::RGBA16;
            default:
                LUMOS_ASSERT(false, "[Texture] Unsupported image bit-depth! ({0})", bits);
                return TextureFormat::RGB8;
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

        TextureCube* TextureCube::Create(uint32_t size)
        {
            LUMOS_ASSERT(CreateFunc, "No TextureCube Create Function");

            return CreateFunc(size);
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

        TextureCube* TextureCube::CreateFromVCross(const std::string* files, uint32_t mips, TextureParameters params, TextureLoadOptions loadOptions, InputFormat format)
        {
            LUMOS_ASSERT(CreateFromVCrossFunc, "No TextureCube Create Function");

            return CreateFromVCrossFunc(files, mips, params, loadOptions, format);
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
