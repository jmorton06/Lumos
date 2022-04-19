#pragma once
#include "Definitions.h"
#include "Core/Asset.h"

namespace Lumos
{
    namespace Graphics
    {
        class LUMOS_EXPORT Texture : public Asset
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

            virtual uint32_t GetWidth() const = 0;
            virtual uint32_t GetHeight() const = 0;
            virtual TextureType GetType() const = 0;
            virtual TextureFormat GetFormat() const = 0;

            virtual uint32_t GetSize() const
            {
                return 0;
            }
            virtual uint32_t GetMipMapLevels() const
            {
                return 0;
            }
            virtual void* GetHandle() const = 0;
            virtual void* GetImageHande() const { return GetHandle(); };

            static bool IsDepthStencilFormat(TextureFormat format)
            {
                return format == TextureFormat::D24_Unorm_S8_UInt || format == TextureFormat::D16_Unorm_S8_UInt || format == TextureFormat::D32_Float_S8_UInt;
            }

            static bool IsDepthFormat(TextureFormat format)
            {
                return format == TextureFormat::D16_Unorm || format == TextureFormat::D32_Float || format == TextureFormat::D24_Unorm_S8_UInt || format == TextureFormat::D16_Unorm_S8_UInt || format == TextureFormat::D32_Float_S8_UInt;
            }

            static bool IsStencilFormat(TextureFormat format)
            {
                return format == TextureFormat::D24_Unorm_S8_UInt || format == TextureFormat::D16_Unorm_S8_UInt || format == TextureFormat::D32_Float_S8_UInt;
            }

            bool IsSampled() const { return m_Flags & Texture_Sampled; }
            bool IsStorage() const { return m_Flags & Texture_Storage; }
            bool IsDepthStencil() const { return m_Flags & Texture_DepthStencil; }
            bool IsRenderTarget() const { return m_Flags & Texture_RenderTarget; }

            virtual void* GetDescriptorInfo() const { return GetHandle(); }

            uint32_t GetBitsPerChannel() const { return m_BitsPerChannel; }
            void SetBitsPerChannel(const uint32_t bits) { m_BitsPerChannel = bits; }
            uint32_t GetBytesPerChannel() const { return m_BitsPerChannel / 8; }
            uint32_t GetBytesPerPixel() const { return (m_BitsPerChannel / 8) * m_ChannelCount; }

        public:
            static uint8_t GetStrideFromFormat(TextureFormat format);
            static TextureFormat BitsToTextureFormat(uint32_t bits);
            static uint32_t BitsToChannelCount(uint32_t bits);
            static uint32_t CalculateMipMapCount(uint32_t width, uint32_t height);

            SET_ASSET_TYPE(AssetType::Texture);

        protected:
            uint16_t m_Flags = 0;
            uint32_t m_BitsPerChannel = 8;
            uint32_t m_ChannelCount = 4;
        };

        class LUMOS_EXPORT Texture2D : public Texture
        {
        public:
            virtual void SetData(const void* pixels) = 0;

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
            static TextureCube* Create(uint32_t size, void* data, bool hdr = false);
            static TextureCube* CreateFromFile(const std::string& filepath);
            static TextureCube* CreateFromFiles(const std::string* files);
            static TextureCube* CreateFromVCross(const std::string* files, uint32_t mips, TextureParameters params, TextureLoadOptions loadOptions, InputFormat = InputFormat::VERTICAL_CROSS);

        protected:
            static TextureCube* (*CreateFunc)(uint32_t, void*, bool);
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
