#pragma once
#include "Graphics/RHI/Texture.h"

namespace Lumos
{
    namespace Graphics
    {
        class GLTexture2D : public Texture2D
        {
        public:
            GLTexture2D(TextureDesc parameters, uint32_t width, uint32_t height);
            GLTexture2D(uint32_t width, uint32_t height, void* data, TextureDesc parameters = TextureDesc(), TextureLoadOptions loadOptions = TextureLoadOptions());
            GLTexture2D(const std::string& name, const std::string& filename, TextureDesc parameters = TextureDesc(), TextureLoadOptions loadOptions = TextureLoadOptions());
            ~GLTexture2D();

            void Bind(uint32_t slot = 0) const override;
            void Unbind(uint32_t slot = 0) const override;

            void Load(uint32_t width, uint32_t height, void* data, TextureDesc parameters = TextureDesc(), TextureLoadOptions loadOptions = TextureLoadOptions()) override;

            virtual void SetData(const void* pixels) override;

            virtual void* GetHandle() const override
            {
                return (void*)(size_t)m_Handle;
            }

            inline uint32_t GetWidth(uint32_t mip) const override
            {
                return m_Width >> mip;
            }

            inline uint32_t GetHeight(uint32_t mip) const override
            {
                return m_Height >> mip;
            }

            inline const std::string& GetName() const override
            {
                return m_Name;
            }
            inline const std::string& GetFilepath() const override
            {
                return m_FileName;
            }

            uint32_t GetMipMapLevels() const override
            {
                return m_MipLevels;
            }

            void Resize(uint32_t width, uint32_t height) override;
            void BuildTexture();

            uint8_t* LoadTextureData();
            uint32_t LoadTexture(void* data);

            TextureType GetType() const override
            {
                return TextureType::COLOUR;
            }

            RHIFormat GetFormat() const override
            {
                return m_Format;
            }

            static void MakeDefault();

        protected:
            static Texture2D* CreateFuncGL(TextureDesc parameters, uint32_t width, uint32_t height);
            static Texture2D* CreateFromSourceFuncGL(uint32_t, uint32_t, void*, TextureDesc, TextureLoadOptions);
            static Texture2D* CreateFromFileFuncGL(const std::string&, const std::string&, TextureDesc, TextureLoadOptions);

        private:
            uint32_t Load(void* data);

            std::string m_Name;
            std::string m_FileName;
            uint32_t m_Handle;
            uint32_t m_Width, m_Height;
            TextureDesc m_Parameters;
            TextureLoadOptions m_LoadOptions;
            RHIFormat m_Format;
            bool isHDR           = false;
            uint32_t m_MipLevels = 1;
        };

        class GLTextureCube : public TextureCube
        {
        public:
            GLTextureCube(uint32_t size, uint8_t* data, bool hdr);
            GLTextureCube(const std::string& filepath);
            GLTextureCube(const std::string* files);
            GLTextureCube(const std::string* files, uint32_t mips, TextureDesc params, TextureLoadOptions loadOptions);
            ~GLTextureCube();

            inline void* GetHandle() const override
            {
                return (void*)(size_t)m_Handle;
            }

            void Bind(uint32_t slot = 0) const override;
            void Unbind(uint32_t slot = 0) const override;

            uint32_t GetMipMapLevels() const override
            {
                return m_NumMips;
            }

            inline uint32_t GetSize() const override
            {
                return m_Size;
            }
            inline const std::string& GetName() const override
            {
                return m_Name;
            }
            inline const std::string& GetFilepath() const override
            {
                return m_Files[0];
            }

            inline uint32_t GetWidth(uint32_t mip) const override
            {
                return m_Width >> mip;
            }

            inline uint32_t GetHeight(uint32_t mip) const override
            {
                return m_Height >> mip;
            }

            TextureType GetType() const override
            {
                return TextureType::CUBE;
            }

            RHIFormat GetFormat() const override
            {
                return m_Format;
            }

            void GenerateMipMaps(CommandBuffer* commandBuffer) override;

            static void MakeDefault();

        protected:
            static TextureCube* CreateFuncGL(uint32_t, void*, bool);
            static TextureCube* CreateFromFileFuncGL(const std::string& filepath);
            static TextureCube* CreateFromFilesFuncGL(const std::string* files);
            static TextureCube* CreateFromVCrossFuncGL(const std::string* files, uint32_t mips, TextureDesc params, TextureLoadOptions loadOptions);

        private:
            static uint32_t LoadFromSingleFile();
            uint32_t LoadFromMultipleFiles();
            uint32_t LoadFromVCross(uint32_t mips);

            uint32_t m_Handle;
            uint32_t m_Width, m_Height, m_Size;
            std::string m_Name;
            std::string m_Files[MAX_MIPS];
            uint32_t m_Bits;
            uint32_t m_NumMips;
            RHIFormat m_Format;
            TextureDesc m_Parameters;
            TextureLoadOptions m_LoadOptions;
        };

        class GLTextureDepth : public TextureDepth
        {
        public:
            GLTextureDepth(uint32_t width, uint32_t height, RHIFormat format);
            ~GLTextureDepth();

            void Bind(uint32_t slot = 0) const override;
            void Unbind(uint32_t slot = 0) const override;
            void Resize(uint32_t width, uint32_t height) override;

            inline void* GetHandle() const override
            {
                return (void*)(size_t)m_Handle;
            }

            inline const std::string& GetName() const override
            {
                return m_Name;
            }
            inline const std::string& GetFilepath() const override
            {
                return m_Name;
            }

            inline uint32_t GetWidth(uint32_t mip) const override
            {
                return m_Width >> mip;
            }

            inline uint32_t GetHeight(uint32_t mip) const override
            {
                return m_Height >> mip;
            }

            TextureType GetType() const override
            {
                return TextureType::DEPTH;
            }

            RHIFormat GetFormat() const override
            {
                return m_Format;
            }

            static void MakeDefault();

        protected:
            static TextureDepth* CreateFuncGL(uint32_t, uint32_t, RHIFormat);

            void Init();

            std::string m_Name;
            uint32_t m_Handle;
            uint32_t m_Width, m_Height;
            RHIFormat m_Format;
        };

        class GLTextureDepthArray : public TextureDepthArray
        {
        private:
            std::string m_Name;
            uint32_t m_Handle;
            uint32_t m_Width, m_Height, m_Count;

        public:
            GLTextureDepthArray(uint32_t width, uint32_t height, uint32_t count, RHIFormat format);
            ~GLTextureDepthArray();

            void Bind(uint32_t slot = 0) const override;
            void Unbind(uint32_t slot = 0) const override;
            void Resize(uint32_t width, uint32_t height, uint32_t count) override;

            inline void* GetHandle() const override
            {
                return (void*)(size_t)m_Handle;
            }

            inline const std::string& GetName() const override
            {
                return m_Name;
            }

            inline const std::string& GetFilepath() const override
            {
                return m_Name;
            }

            inline uint32_t GetWidth(uint32_t mip) const override
            {
                return m_Width >> mip;
            }

            inline uint32_t GetHeight(uint32_t mip) const override
            {
                return m_Height >> mip;
            }

            inline void SetCount(uint32_t count)
            {
                m_Count = count;
            }

            TextureType GetType() const override
            {
                return TextureType::DEPTHARRAY;
            }

            RHIFormat GetFormat() const override
            {
                return m_Format;
            }

            uint32_t GetCount() const override { return m_Count; }

            void Init() override;

            static void MakeDefault();

        protected:
            static TextureDepthArray* CreateFuncGL(uint32_t, uint32_t, uint32_t, RHIFormat);
            RHIFormat m_Format;
        };
    }
}
