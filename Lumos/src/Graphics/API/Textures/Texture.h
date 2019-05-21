#pragma once

#include "LM.h"

namespace Lumos
{

	enum class LUMOS_EXPORT TextureWrap
	{
		NONE = 0,
		REPEAT,
		CLAMP,
		MIRRORED_REPEAT,
		CLAMP_TO_EDGE,
		CLAMP_TO_BORDER
	};

	enum class LUMOS_EXPORT TextureFilter
	{
		NONE = 0,
		LINEAR,
		NEAREST
	};

	enum class LUMOS_EXPORT TextureFormat
	{
		NONE = 0,
		R8,
		RG8,
		RGB8,
		RGBA8,
        RGB16,
        RGBA16,
		RGB,
		RGBA,
		LUMINANCE,
		LUMINANCE_ALPHA,
		DEPTH
        };

	enum class LUMOS_EXPORT TextureType
	{
		DEPTH,
		COLOUR,
		DEPTHARRAY,
		CUBE,
		OTHER
	};

	struct LUMOS_EXPORT TextureParameters
	{
		TextureFormat format;
		TextureFilter filter;
		TextureWrap wrap;

		TextureParameters()
		{
			format = TextureFormat::RGBA;
			filter = TextureFilter::NEAREST;
			wrap = TextureWrap::REPEAT;
		}

		TextureParameters(TextureFormat format, TextureFilter filter, TextureWrap wrap)
			: format(format), filter(filter), wrap(wrap)
		{
		}

		TextureParameters(TextureFilter filter)
			: format(TextureFormat::RGBA), filter(filter), wrap(TextureWrap::CLAMP)
		{
		}

		TextureParameters(TextureFilter filter, TextureWrap wrap)
			: format(TextureFormat::RGBA), filter(filter), wrap(wrap)
		{
		}

		TextureParameters(TextureWrap wrap)
			: format(TextureFormat::RGBA), filter(TextureFilter::LINEAR), wrap(wrap)
		{
		}

		TextureParameters(TextureFormat format)
			: format(format), filter(TextureFilter::LINEAR), wrap(TextureWrap::CLAMP)
		{
		}
	};

	struct LUMOS_EXPORT TextureLoadOptions
	{
		bool flipX;
		bool flipY;
		bool generateMipMaps;

		TextureLoadOptions()
		{
			flipX = false;
			flipY = false;
			generateMipMaps = true;
		}

		TextureLoadOptions(bool flipX, bool flipY, bool genMips = true)
			: flipX(flipX), flipY(flipY), generateMipMaps(genMips)
		{
		}
	};

	class LUMOS_EXPORT Texture
	{
	protected:
		static TextureWrap s_WrapMode;
		static TextureFilter s_FilterMode;
	public:
		virtual ~Texture() {}

		virtual void Bind(uint slot = 0) const = 0;
		virtual void Unbind(uint slot = 0) const = 0;

		virtual const String& GetName() const = 0;
		virtual const String& GetFilepath() const = 0;

		virtual uint GetSize() const { return 0; }
		virtual void* GetHandle() const = 0;

	public:
		inline static void SetWrap(const TextureWrap mode) { s_WrapMode = mode; }
		inline static void SetFilter(const TextureFilter mode) { s_FilterMode = mode; }
	public:
		static byte GetStrideFromFormat(TextureFormat format);
	};
}
