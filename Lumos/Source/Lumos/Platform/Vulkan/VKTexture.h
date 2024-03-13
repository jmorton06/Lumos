#pragma once
#include "Graphics/RHI/Texture.h"
#include "VK.h"
#include "Graphics/RHI/GraphicsContext.h"
#include "VKContext.h"
#include "VKCommandBuffer.h"
#include "VKBuffer.h"

namespace Lumos
{
    namespace Graphics
    {
#ifdef USE_VMA_ALLOCATOR
        void CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageType imageType, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, uint32_t arrayLayers, VkImageCreateFlags flags, VmaAllocation& allocation, uint32_t samples = 1);
#else
        void CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageType imageType, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, uint32_t arrayLayers, VkImageCreateFlags flags, uint32_t samples = 1);
#endif

        class VKTexture2D : public Texture2D
        {
        public:
            VKTexture2D(TextureDesc parameters, uint32_t width, uint32_t height);
            VKTexture2D(uint32_t width, uint32_t height, void* data, TextureDesc parameters = TextureDesc(), TextureLoadOptions loadOptions = TextureLoadOptions());
            VKTexture2D(const std::string& name, const std::string& filename, TextureDesc parameters = TextureDesc(), TextureLoadOptions loadOptions = TextureLoadOptions());
            VKTexture2D(VkImage image, VkImageView imageView, VkFormat format, uint32_t width, uint32_t height);
            ~VKTexture2D();

            void DeleteResources();

            void Bind(uint32_t slot = 0) const override {};
            void Unbind(uint32_t slot = 0) const override {};
            void Load(uint32_t width, uint32_t height, void* data, TextureDesc parameters = TextureDesc(), TextureLoadOptions loadOptions = TextureLoadOptions()) override;

            virtual void SetData(const void* pixels) override;

            virtual void* GetHandle() const override
            {
                return (void*)this;
            }

            inline uint32_t GetWidth(uint32_t mip = 0) const override
            {
                return m_Width >> mip;
            }

            inline uint32_t GetHeight(uint32_t mip = 0) const override
            {
                return m_Height >> mip;
            }

            uint32_t GetMipMapLevels() const override
            {
                return m_MipLevels;
            }

            inline const std::string& GetName() const override
            {
                return m_Name;
            }

            inline const std::string& GetFilepath() const override
            {
                return m_FileName;
            }

            TextureType GetType() const override
            {
                return TextureType::COLOUR;
            }

            RHIFormat GetFormat() const override
            {
                return m_Format;
            }

            void SetName(const std::string& name) override
            {
                m_Name = name;
            }

            void BuildTexture();
            void Resize(uint32_t width, uint32_t height) override;

            const VkDescriptorImageInfo* GetDescriptor() const
            {
                return &m_Descriptor;
            }

            void SetImageLayout(VkImageLayout layout)
            {
                m_ImageLayout = layout;
                UpdateDescriptor();
            }

            VkDescriptorImageInfo& GetDescriptorRef()
            {
                return m_Descriptor;
            }

            void* GetDescriptorInfo() const override
            {
                return (void*)GetDescriptor();
            }

            virtual void* GetImageHande() const override
            {
                return (void*)m_TextureImage;
            }

            void UpdateDescriptor();

            bool Load();

            VkImage GetImage() const
            {
                return m_TextureImage;
            };
            VkDeviceMemory GetDeviceMemory() const
            {
                return m_TextureImageMemory;
            }
            VkImageView GetImageView() const
            {
                return m_TextureImageView;
            }
            VkSampler GetSampler() const
            {
                return m_TextureSampler;
            }

            VkFormat GetVKFormat()
            {
                return m_VKFormat;
            }

			uint8_t GetSamples() const override
			{
				return m_Samples;
			}

            VkImageView GetMipImageView(uint32_t mip);

            VkImageLayout GetImageLayout() const { return m_ImageLayout; }
            const TextureDesc& GetTextureParameters() const { return m_Parameters; }
            void TransitionImage(VkImageLayout newLayout, VKCommandBuffer* commandBuffer = nullptr);

            static void MakeDefault();

        protected:
            static Texture2D* CreateFuncVulkan(TextureDesc, uint32_t, uint32_t);
            static Texture2D* CreateFromSourceFuncVulkan(uint32_t, uint32_t, void*, TextureDesc, TextureLoadOptions);
            static Texture2D* CreateFromFileFuncVulkan(const std::string&, const std::string&, TextureDesc, TextureLoadOptions);

        private:
            std::string m_Name;
            std::string m_FileName;
            uint32_t m_Width {}, m_Height {};
            uint32_t m_MipLevels = 1;
            uint8_t* m_Data      = nullptr;
            uint8_t m_Samples = 1;

            TextureDesc m_Parameters;
            TextureLoadOptions m_LoadOptions;

            RHIFormat m_Format;
            VkFormat m_VKFormat = VK_FORMAT_R8G8B8A8_UNORM;

            VkImage m_TextureImage {};
            VkImageLayout m_ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            VkDeviceMemory m_TextureImageMemory {};
            VkImageView m_TextureImageView;
            VkSampler m_TextureSampler {};
            VkDescriptorImageInfo m_Descriptor {};

            std::unordered_map<uint32_t, VkImageView> m_MipImageViews;

#ifdef USE_VMA_ALLOCATOR
            VmaAllocation m_Allocation {};
#endif

            bool m_DeleteImage = true;
        };

        class VKTextureCube : public TextureCube
        {
        public:
            VKTextureCube(uint32_t size, void* data, bool hdr);
            VKTextureCube(const std::string& filepath);
            VKTextureCube(const std::string* files);
            VKTextureCube(const std::string* files, uint32_t mips, TextureDesc params, TextureLoadOptions loadOptions);
            ~VKTextureCube();

            virtual void* GetHandle() const override
            {
                return (void*)this;
            }

            void TransitionImage(VkImageLayout newLayout, VKCommandBuffer* commandBuffer);

            void Bind(uint32_t slot = 0) const override {};
            void Unbind(uint32_t slot = 0) const override {};

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

            uint32_t GetMipMapLevels() const override
            {
                return m_NumMips;
            }

            TextureType GetType() const override
            {
                return TextureType::CUBE;
            }

            RHIFormat GetFormat() const override
            {
                return m_Format;
            }

            const VkDescriptorImageInfo* GetDescriptor() const
            {
                return &m_Descriptor;
            }

            void* GetDescriptorInfo() const override
            {
                return (void*)GetDescriptor();
            }

            void UpdateDescriptor();

            void Load(uint32_t mips);

            VkImage GetImage() const
            {
                return m_TextureImage;
            }

            virtual void* GetImageHande() const override
            {
                return (void*)m_TextureImage;
            }

            VkDeviceMemory GetDeviceMemory() const
            {
                return m_TextureImageMemory;
            }

            VkImageView GetImageView() const
            {
                return m_TextureImageView;
            }

            VkImageView GetImageView(uint32_t layer) const
            {
                // return m_IndividualImageViews[layer];
                return m_ImageViewsPerMip[0 + layer];
            }

            VkImageView GetImageView(uint32_t layer, uint32_t mip) const
            {
                return m_ImageViewsPerMip[mip * 6 + layer];
            }

            VkSampler GetSampler() const
            {
                return m_TextureSampler;
            }

            VkFormat GetVKFormat() const
            {
                return m_VKFormat;
            }

            VkImageLayout GetImageLayout() const { return m_ImageLayout; }

            void GenerateMipMaps(CommandBuffer* commandBuffer) override;
            void Destroy(bool useDeletionQueue) override;

            static void MakeDefault();

        protected:
            static TextureCube* CreateFuncVulkan(uint32_t, void* data, bool hdr);
            static TextureCube* CreateFromFileFuncVulkan(const std::string& filepath);
            static TextureCube* CreateFromFilesFuncVulkan(const std::string* files);
            static TextureCube* CreateFromVCrossFuncVulkan(const std::string* files, uint32_t mips, TextureDesc params, TextureLoadOptions loadOptions);

        private:
            std::string m_Name;
            std::string m_Files[MAX_MIPS];
            uint32_t m_Handle    = 0;
            uint32_t m_Width     = 0;
            uint32_t m_Height    = 0;
            uint32_t m_Size      = 0;
            uint32_t m_NumMips   = 0;
            uint32_t m_NumLayers = 6;
            uint8_t* m_Data      = nullptr;

            TextureDesc m_Parameters;
            TextureLoadOptions m_LoadOptions;

            RHIFormat m_Format;
            VkFormat m_VKFormat;

            VkImage m_TextureImage {};
            VkImageLayout m_ImageLayout;
            VkDeviceMemory m_TextureImageMemory {};
            VkImageView m_TextureImageView {};
            VkSampler m_TextureSampler {};
            VkDescriptorImageInfo m_Descriptor {};
            std::vector<VkImageView> m_IndividualImageViews;
            std::vector<VkImageView> m_ImageViewsPerMip;

#ifdef USE_VMA_ALLOCATOR
            VmaAllocation m_Allocation {};
#endif

            bool m_DeleteImage = true;
        };

        class VKTextureDepth : public TextureDepth
        {
        public:
            VKTextureDepth(uint32_t width, uint32_t height, RHIFormat format, uint8_t samples);
            ~VKTextureDepth();

            void Bind(uint32_t slot = 0) const override {};
            void Unbind(uint32_t slot = 0) const override {};
            void Resize(uint32_t width, uint32_t height) override;

            inline uint32_t GetWidth(uint32_t mip) const override
            {
                return m_Width >> mip;
            }

            inline uint32_t GetHeight(uint32_t mip) const override
            {
                return m_Height >> mip;
            }

            virtual void* GetHandle() const override
            {
                return (void*)this;
            }

            virtual void* GetImageHande() const override
            {
                return (void*)m_TextureImage;
            }

            inline const std::string& GetName() const override
            {
                return m_Name;
            }
            inline const std::string& GetFilepath() const override
            {
                return m_Name;
            }

            TextureType GetType() const override
            {
                return TextureType::DEPTH;
            }

            RHIFormat GetFormat() const override
            {
                return m_Format;
            }

			uint8_t GetSamples() const override
			{
				return m_Samples;
			}

            VkImage GetImage() const
            {
                return m_TextureImage;
            };
            VkDeviceMemory GetDeviceMemory() const
            {
                return m_TextureImageMemory;
            }
            VkImageView GetImageView() const
            {
                return m_TextureImageView;
            }
            VkSampler GetSampler() const
            {
                return m_TextureSampler;
            }
            const VkDescriptorImageInfo* GetDescriptor() const
            {
                return &m_Descriptor;
            }
            void* GetDescriptorInfo() const override
            {
                return (void*)GetDescriptor();
            }

            VkFormat GetVKFormat() const
            {
                return m_VKFormat;
            }

            void UpdateDescriptor();
            void TransitionImage(VkImageLayout newLayout, VKCommandBuffer* commandBuffer = nullptr);

            VkImageLayout GetImageLayout() const { return m_ImageLayout; }

            static void MakeDefault();

        protected:
            static TextureDepth* CreateFuncVulkan(uint32_t, uint32_t, RHIFormat, uint8_t);
            void Init();

        private:
            std::string m_Name;
            uint32_t m_Handle {};
            uint32_t m_Width, m_Height;
            uint8_t m_Samples;
            RHIFormat m_Format;
            VkFormat m_VKFormat;
            VkImageLayout m_ImageLayout;
            VkImage m_TextureImage {};
            VkDeviceMemory m_TextureImageMemory {};
            VkImageView m_TextureImageView {};
            VkSampler m_TextureSampler {};
            VkDescriptorImageInfo m_Descriptor {};

#ifdef USE_VMA_ALLOCATOR
            VmaAllocation m_Allocation {};
#endif
        };

        class VKTextureDepthArray : public TextureDepthArray
        {
        public:
            VKTextureDepthArray(uint32_t width, uint32_t height, uint32_t count, RHIFormat format);
            ~VKTextureDepthArray();

            void Bind(uint32_t slot = 0) const override {};
            void Unbind(uint32_t slot = 0) const override {};
            void Resize(uint32_t width, uint32_t height, uint32_t count) override;

            inline uint32_t GetWidth(uint32_t mip) const override
            {
                return m_Width >> mip;
            }

            inline uint32_t GetHeight(uint32_t mip) const override
            {
                return m_Height >> mip;
            }

            virtual void* GetHandle() const override
            {
                return (void*)this;
            }

            inline const std::string& GetName() const override
            {
                return m_Name;
            }

            inline const std::string& GetFilepath() const override
            {
                return m_Name;
            }

            TextureType GetType() const override
            {
                return TextureType::DEPTHARRAY;
            }

            RHIFormat GetFormat() const override
            {
                return m_Format;
            }

            VkImage GetImage() const
            {
                return m_TextureImage;
            }

            VkDeviceMemory GetDeviceMemory() const
            {
                return m_TextureImageMemory;
            }

            VkImageView GetImageView() const
            {
                return m_TextureImageView;
            }

            VkImageView GetImageView(int index) const
            {
                return m_IndividualImageViews[index];
            }

            VkSampler GetSampler() const
            {
                return m_TextureSampler;
            }

            const VkDescriptorImageInfo* GetDescriptor() const
            {
                return &m_Descriptor;
            }

            VkFormat GetVKFormat() const
            {
                return m_VKFormat;
            }

            void* GetDescriptorInfo() const override
            {
                return (void*)GetDescriptor();
            }

            virtual void* GetImageHande() const override
            {
                return (void*)m_TextureImage;
            }

            void UpdateDescriptor();
            void TransitionImage(VkImageLayout newLayout, VKCommandBuffer* commandBuffer = nullptr);

            void* GetHandleArray(uint32_t index) override;

            uint32_t GetCount() const override { return m_Count; }
            VkImageLayout GetImageLayout() const { return m_ImageLayout; }

            static void MakeDefault();

        protected:
            static TextureDepthArray* CreateFuncVulkan(uint32_t, uint32_t, uint32_t, RHIFormat);
            void Init() override;

        private:
            std::string m_Name;
            uint32_t m_Handle {};
            uint32_t m_Width, m_Height;
            uint32_t m_Count;

            RHIFormat m_Format;

            VkFormat m_VKFormat;
            VkImageLayout m_ImageLayout;
            VkImage m_TextureImage {};
            VkDeviceMemory m_TextureImageMemory {};
            VkImageView m_TextureImageView {};
            VkSampler m_TextureSampler {};
            VkDescriptorImageInfo m_Descriptor {};
            std::vector<VkImageView> m_IndividualImageViews;

#ifdef USE_VMA_ALLOCATOR
            VmaAllocation m_Allocation {};
#endif
        };
    }
}
