#pragma once

#define MAX_MIPS 11

namespace Lumos
{
	namespace Graphics
	{
		enum class TextureWrap
		{
			NONE,
			REPEAT,
			CLAMP,
			MIRRORED_REPEAT,
			CLAMP_TO_EDGE,
			CLAMP_TO_BORDER
		};

		enum class TextureFilter
		{
			NONE,
			LINEAR,
			NEAREST
		};

		enum class TextureFormat
		{
			NONE,
			R8,
			RG8,
			RGB8,
			RGBA8,
			RGB16,
			RGBA16,
			RGB32,
			RGBA32,
			RGB,
			RGBA,
			DEPTH,
			STENCIL,
			DEPTH_STENCIL
		};

		enum class TextureType
		{
			COLOUR,
			DEPTH,
			DEPTHARRAY,
			CUBE,
			OTHER
		};

		struct TextureParameters
		{
			TextureFormat format;
			TextureFilter minFilter;
			TextureFilter magFilter;
			TextureWrap wrap;
            bool srgb = false;

			TextureParameters()
			{
				format = TextureFormat::RGBA8;
				minFilter = TextureFilter::NEAREST;
				magFilter = TextureFilter::NEAREST;
				wrap = TextureWrap::REPEAT;
			}

			TextureParameters(TextureFormat format, TextureFilter minFilter, TextureFilter magFilter, TextureWrap wrap)
				: format(format)
				, minFilter(minFilter)
				, magFilter(magFilter)
				, wrap(wrap)
			{
			}

			TextureParameters(TextureFilter minFilter, TextureFilter magFilter)
				: format(TextureFormat::RGBA8)
				, minFilter(minFilter)
				, magFilter(magFilter)
				, wrap(TextureWrap::CLAMP)
			{
			}

			TextureParameters(TextureFilter minFilter, TextureFilter magFilter, TextureWrap wrap)
				: format(TextureFormat::RGBA8)
				, minFilter(minFilter)
				, magFilter(magFilter)
				, wrap(wrap)
			{
			}

			TextureParameters(TextureWrap wrap)
				: format(TextureFormat::RGBA8)
				, minFilter(TextureFilter::LINEAR)
				, magFilter(TextureFilter::LINEAR)
				, wrap(wrap)
			{
			}

			TextureParameters(TextureFormat format)
				: format(format)
				, minFilter(TextureFilter::LINEAR)
				, magFilter(TextureFilter::LINEAR)
				, wrap(TextureWrap::CLAMP)
			{
			}
		};

		struct TextureLoadOptions
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
				: flipX(flipX)
				, flipY(flipY)
				, generateMipMaps(genMips)
			{
			}
		};

		class LUMOS_EXPORT Texture
		{
		public:
			virtual ~Texture()
			{
			}

			virtual void Bind(u32 slot = 0) const = 0;
			virtual void Unbind(u32 slot = 0) const = 0;

			virtual void SetName(const std::string& name) {};
			virtual const std::string& GetName() const = 0;
			virtual const std::string& GetFilepath() const = 0;

			virtual u32 GetSize() const
			{
				return 0;
			}
			virtual u32 GetMipMapLevels() const
			{
				return 0;
			}
			virtual void* GetHandle() const = 0;

			static bool IsDepthStencilFormat(TextureFormat format)
			{
				return format == TextureFormat::DEPTH_STENCIL;
			}

			static bool IsDepthFormat(TextureFormat format)
			{
				return format == TextureFormat::DEPTH;
			}

			static bool IsStencilFormat(TextureFormat format)
			{
				return format == TextureFormat::STENCIL;
			}

		public:
			static u8 GetStrideFromFormat(TextureFormat format);
			static TextureFormat BitsToTextureFormat(u32 bits);
			static u32 CalculateMipMapCount(u32 width, u32 height);
		};

		class LUMOS_EXPORT Texture2D : public Texture
		{
		public:
			virtual void SetData(const void* pixels) = 0;

			virtual u32 GetWidth() const = 0;
			virtual u32 GetHeight() const = 0;

		public:
			static Texture2D* Create();
			static Texture2D* CreateFromSource(u32 width, u32 height, void* data, TextureParameters parameters = TextureParameters(), TextureLoadOptions loadOptions = TextureLoadOptions());
			static Texture2D* CreateFromFile(const std::string& name, const std::string& filepath, TextureParameters parameters = TextureParameters(), TextureLoadOptions loadOptions = TextureLoadOptions());

			virtual void BuildTexture(TextureFormat internalformat, u32 width, u32 height, bool srgb = false, bool depth = false, bool samplerShadow = false) = 0;

		protected:
			static Texture2D* (*CreateFunc)();
			static Texture2D* (*CreateFromSourceFunc)(u32, u32, void*, TextureParameters, TextureLoadOptions);
			static Texture2D* (*CreateFromFileFunc)(const std::string&, const std::string&, TextureParameters, TextureLoadOptions);
		};

		class LUMOS_EXPORT TextureCube : public Texture
		{
		protected:
			enum class InputFormat
			{
				VERTICAL_CROSS,
				HORIZONTAL_CROSS
			};

		public:
			static TextureCube* Create(u32 size);
			static TextureCube* CreateFromFile(const std::string& filepath);
			static TextureCube* CreateFromFiles(const std::string* files);
			static TextureCube* CreateFromVCross(const std::string* files, u32 mips, TextureParameters params, TextureLoadOptions loadOptions, InputFormat = InputFormat::VERTICAL_CROSS);

		protected:
			static TextureCube* (*CreateFunc)(u32);
			static TextureCube* (*CreateFromFileFunc)(const std::string&);
			static TextureCube* (*CreateFromFilesFunc)(const std::string*);
			static TextureCube* (*CreateFromVCrossFunc)(const std::string*, u32, TextureParameters, TextureLoadOptions, InputFormat);
		};

		class LUMOS_EXPORT TextureDepth : public Texture
		{
		public:
			static TextureDepth* Create(u32 width, u32 height);

			virtual void Resize(u32 width, u32 height) = 0;

		protected:
			static TextureDepth* (*CreateFunc)(u32, u32);
		};

		class LUMOS_EXPORT TextureDepthArray : public Texture
		{
		public:
			static TextureDepthArray* Create(u32 width, u32 height, u32 count);

			virtual void Init() = 0;
			virtual void Resize(u32 width, u32 height, u32 count) = 0;
			virtual void* GetHandleArray(u32 index)
			{
				return GetHandle();
			};

		protected:
			static TextureDepthArray* (*CreateFunc)(u32, u32, u32);
		};
	}
}
