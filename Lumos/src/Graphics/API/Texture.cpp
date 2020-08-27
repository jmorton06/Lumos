#include "Precompiled.h"
#include "Texture.h"

#include "Utilities/LoadImage.h"

namespace Lumos
{
	namespace Graphics
	{
		Texture2D* (*Texture2D::CreateFunc)() = nullptr;
		Texture2D* (*Texture2D::CreateFromSourceFunc)(u32, u32, void*, TextureParameters, TextureLoadOptions) = nullptr;
		Texture2D* (*Texture2D::CreateFromFileFunc)(const std::string&, const std::string&, TextureParameters, TextureLoadOptions) = nullptr;

		TextureDepth* (*TextureDepth::CreateFunc)(u32, u32) = nullptr;
		TextureDepthArray* (*TextureDepthArray::CreateFunc)(u32, u32, u32) = nullptr;

		TextureCube* (*TextureCube::CreateFunc)(u32) = nullptr;
		TextureCube* (*TextureCube::CreateFromFileFunc)(const std::string&) = nullptr;
		TextureCube* (*TextureCube::CreateFromFilesFunc)(const std::string*) = nullptr;
		TextureCube* (*TextureCube::CreateFromVCrossFunc)(const std::string*, u32, InputFormat) = nullptr;

		u8 Texture::GetStrideFromFormat(const TextureFormat format)
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

		TextureFormat Texture::BitsToTextureFormat(u32 bits)
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

		u32 Texture::CalculateMipMapCount(u32 width, u32 height)
		{
			u32 levels = 1;
			while((width | height) >> levels)
				levels++;

			return levels;
		}

		Texture2D* Texture2D::Create()
		{
			LUMOS_ASSERT(CreateFunc, "No Texture2D Create Function");

			return CreateFunc();
		}

		Texture2D* Texture2D::CreateFromSource(u32 width, u32 height, void* data, TextureParameters parameters, TextureLoadOptions loadOptions)
		{
			LUMOS_ASSERT(CreateFromSourceFunc, "No Texture2D Create Function");

			return CreateFromSourceFunc(width, height, data, parameters, loadOptions);
		}

		Texture2D* Texture2D::CreateFromFile(const std::string& name, const std::string& filepath, TextureParameters parameters, TextureLoadOptions loadOptions)
		{
			LUMOS_ASSERT(CreateFromFileFunc, "No Texture2D Create Function");

			return CreateFromFileFunc(name, filepath, parameters, loadOptions);
		}

		TextureCube* TextureCube::Create(u32 size)
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

		TextureCube* TextureCube::CreateFromVCross(const std::string* files, u32 mips, InputFormat format)
		{
			LUMOS_ASSERT(CreateFromVCrossFunc, "No TextureCube Create Function");

			return CreateFromVCrossFunc(files, mips, format);
		}

		TextureDepth* TextureDepth::Create(u32 width, u32 height)
		{
			LUMOS_ASSERT(CreateFunc, "No TextureDepth Create Function");

			return CreateFunc(width, height);
		}

		TextureDepthArray* TextureDepthArray::Create(u32 width, u32 height, u32 count)
		{
			LUMOS_ASSERT(CreateFunc, "No TextureDepthArray Create Function");

			return CreateFunc(width, height, count);
		}
	}
}
