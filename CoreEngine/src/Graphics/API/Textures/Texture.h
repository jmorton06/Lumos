#pragma once

#include "JM.h"

namespace jm
{

	enum class JM_EXPORT TextureWrap
	{
		NONE = 0,
		REPEAT,
		CLAMP,
		MIRRORED_REPEAT,
		CLAMP_TO_EDGE,
		CLAMP_TO_BORDER
	};

	enum class JM_EXPORT TextureFilter
	{
		NONE = 0,
		LINEAR,
		NEAREST
	};

	enum class JM_EXPORT TextureFormat
	{
		NONE = 0,
		R8,
		RG8,
		RGB8,
		RGBA8,
		RGB,
		RGBA,
		LUMINANCE,
		LUMINANCE_ALPHA,
		DEPTH,
		RGB16
	};

	enum class JM_EXPORT TextureType
	{
		DEPTH,
		COLOUR,
		DEPTHARRAY,
		CUBE,
		OTHER
	};

	struct JM_EXPORT TextureParameters
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

	struct JM_EXPORT TextureLoadOptions
	{
		bool flipX;
		bool flipY;

		TextureLoadOptions()
		{
			flipX = false;
			flipY = false;
		}

		TextureLoadOptions(bool flipX, bool flipY)
			: flipX(flipX), flipY(flipY)
		{
		}
	};

	class JM_EXPORT Texture
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
		virtual uint GetHandle() const = 0;

	public:
		inline static void SetWrap(const TextureWrap mode) { s_WrapMode = mode; }
		inline static void SetFilter(const TextureFilter mode) { s_FilterMode = mode; }
	public:
		static byte GetStrideFromFormat(TextureFormat format);
	};
}
