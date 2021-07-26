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
            DEPTH_STENCIL,
            SCREEN
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
            uint32_t msaaLevel;

            TextureParameters()
            {
                format = TextureFormat::RGBA8;
                minFilter = TextureFilter::NEAREST;
                magFilter = TextureFilter::NEAREST;
                wrap = TextureWrap::REPEAT;
                msaaLevel = 1;
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

            virtual void Bind(uint32_t slot = 0) const = 0;
            virtual void Unbind(uint32_t slot = 0) const = 0;

            virtual void SetName(const std::string& name) {};
            virtual const std::string& GetName() const = 0;
            virtual const std::string& GetFilepath() const = 0;

            virtual uint32_t GetSize() const
            {
                return 0;
            }
            virtual uint32_t GetMipMapLevels() const
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
            static uint8_t GetStrideFromFormat(TextureFormat format);
            static TextureFormat BitsToTextureFormat(uint32_t bits);
            static uint32_t CalculateMipMapCount(uint32_t width, uint32_t height);
        };

        class LUMOS_EXPORT Texture2D : public Texture
        {
        public:
            virtual void SetData(const void* pixels) = 0;

            virtual uint32_t GetWidth() const = 0;
            virtual uint32_t GetHeight() const = 0;

        public:
            static Texture2D* Create();
            static Texture2D* CreateFromSource(uint32_t width, uint32_t height, void* data, TextureParameters parameters = TextureParameters(), TextureLoadOptions loadOptions = TextureLoadOptions());
            static Texture2D* CreateFromFile(const std::string& name, const std::string& filepath, TextureParameters parameters = TextureParameters(), TextureLoadOptions loadOptions = TextureLoadOptions());

            virtual void BuildTexture(TextureFormat internalformat, uint32_t width, uint32_t height, bool srgb = false, bool depth = false, bool samplerShadow = false) = 0;

        protected:
            static Texture2D* (*CreateFunc)();
            static Texture2D* (*CreateFromSourceFunc)(uint32_t, uint32_t, void*, TextureParameters, TextureLoadOptions);
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
            static TextureCube* Create(uint32_t size);
            static TextureCube* CreateFromFile(const std::string& filepath);
            static TextureCube* CreateFromFiles(const std::string* files);
            static TextureCube* CreateFromVCross(const std::string* files, uint32_t mips, TextureParameters params, TextureLoadOptions loadOptions, InputFormat = InputFormat::VERTICAL_CROSS);

        protected:
            static TextureCube* (*CreateFunc)(uint32_t);
            static TextureCube* (*CreateFromFileFunc)(const std::string&);
            static TextureCube* (*CreateFromFilesFunc)(const std::string*);
            static TextureCube* (*CreateFromVCrossFunc)(const std::string*, uint32_t, TextureParameters, TextureLoadOptions, InputFormat);
        };

        class LUMOS_EXPORT TextureDepth : public Texture
        {
        public:
            static TextureDepth* Create(uint32_t width, uint32_t height);

            virtual void Resize(uint32_t width, uint32_t height) = 0;

        protected:
            static TextureDepth* (*CreateFunc)(uint32_t, uint32_t);
        };

        class LUMOS_EXPORT TextureDepthArray : public Texture
        {
        public:
            static TextureDepthArray* Create(uint32_t width, uint32_t height, uint32_t count);

            virtual void Init() = 0;
            virtual void Resize(uint32_t width, uint32_t height, uint32_t count) = 0;
            virtual void* GetHandleArray(uint32_t index)
            {
                return GetHandle();
            };

        protected:
            static TextureDepthArray* (*CreateFunc)(uint32_t, uint32_t, uint32_t);
        };
    }
}
