#include "LM.h"
#include "Texture.h"

namespace Lumos
{
	namespace Graphics
	{
		TextureWrap Texture::s_WrapMode = TextureWrap::REPEAT;
		TextureFilter Texture::s_FilterMode = TextureFilter::LINEAR;

		byte Texture::GetStrideFromFormat(const TextureFormat format)
		{
			switch (format)
			{
			case TextureFormat::RGB:				return 3;
			case TextureFormat::RGBA:				return 4;
			case TextureFormat::R8:					return 1;
			case TextureFormat::RG8:				return 2;
			case TextureFormat::RGB8:				return 3;
			case TextureFormat::RGBA8:				return 4;
			default: return 0;
			}
		}
	}
}