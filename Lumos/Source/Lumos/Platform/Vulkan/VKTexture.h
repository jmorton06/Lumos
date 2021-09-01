#pragma once
#include "Graphics/RHI/Texture.h"
#include "VK.h"
#include "Graphics/RHI/GraphicsContext.h"
#include "VKContext.h"
#include "VKCommandBuffer.h"

#ifdef USE_VMA_ALLOCATOR
#include <vulkan/vk_mem_alloc.h>
#endif

namespace Lumos
{
    namespace Graphics
    {
        class VKTexture2D : public Texture2D
        {
        public:
            VKTexture2D(uint32_t width, uint32_t height, void* data, TextureParameters parameters = TextureParameters(), TextureLoadOptions loadOptions = TextureLoadOptions());
            VKTexture2D(const std::string& name, const std::string& filename, TextureParameters parameters = TextureParameters(), TextureLoadOptions loadOptions = TextureLoadOptions());
            VKTexture2D(VkImage image, VkImageView imageView, VkFormat format, uint32_t width, uint32_t height);
            VKTexture2D();
            ~VKTexture2D();

            void Bind(uint32_t slot = 0) const override {};
            void Unbind(uint32_t slot = 0) const override {};

            virtual void SetData(const void* pixels) override {};

            virtual void* GetHandle() const override
            {
                return (void*)this;
            }

            inline uint32_t GetWidth() const override
            {
                return m_Width;
            }
            inline uint32_t GetHeight() const override
            {
                return m_Height;
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

            TextureFormat GetFormat() const override
            {
                return m_Format;
            }

            void SetName(const std::string& name) override
            {
                m_Name = name;
            }

            void BuildTexture(TextureFormat internalformat, uint32_t width, uint32_t height, bool srgb, bool depth, bool samplerShadow) override;

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
            
            VkImageLayout GetImageLayout() const { return m_ImageLayout; }

            const TextureParameters& GetTextureParameters() const { return m_Parameters; }

            void TransitionImage(VkImageLayout newLayout, VKCommandBuffer* commandBuffer = nullptr);

            static void MakeDefault();

        protected:
            static Texture2D* CreateFuncVulkan();
            static Texture2D* CreateFromSourceFuncVulkan(uint32_t, uint32_t, void*, TextureParameters, TextureLoadOptions);
            static Texture2D* CreateFromFileFuncVulkan(const std::string&, const std::string&, TextureParameters, TextureLoadOptions);

        private:
            std::string m_Name;
            std::string m_FileName;
            uint32_t m_Width {}, m_Height {};
            uint32_t m_MipLevels = 1;
            uint8_t* m_Data = nullptr;

            TextureParameters m_Parameters;
            TextureLoadOptions m_LoadOptions;

            TextureFormat m_Format;
            VkFormat m_VKFormat = VK_FORMAT_R8G8B8A8_UNORM;

            VkImage m_TextureImage {};
            VkImageLayout m_ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            VkDeviceMemory m_TextureImageMemory {};
            VkImageView m_TextureImageView;
            VkSampler m_TextureSampler {};
            VkDescriptorImageInfo m_Descriptor {};

#ifdef USE_VMA_ALLOCATOR
            VmaAllocation m_Allocation {};
#endif

            bool m_DeleteImage = true;
        };

        class VKTextureCube : public TextureCube
        {
        public:
            VKTextureCube(uint32_t size);
            VKTextureCube(const std::string& filepath);
            VKTextureCube(const std::string* files);
            VKTextureCube(const std::string* files, uint32_t mips, TextureParameters params, TextureLoadOptions loadOptions, InputFormat format);
            ~VKTextureCube();

            virtual void* GetHandle() const override
            {
                return (void*)this;
            }

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

            inline uint32_t GetWidth() const override
            {
                return m_Width;
            }

            inline uint32_t GetHeight() const override
            {
                return m_Height;
            }

            uint32_t GetMipMapLevels() const override
            {
                return m_NumMips;
            }

            TextureType GetType() const override
            {
                return TextureType::CUBE;
            }

            TextureFormat GetFormat() const override
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
            
            VkFormat GetVKFormat() const
            {
                return m_VKFormat;
            }

            static void MakeDefault();

        protected:
            static TextureCube* CreateFuncVulkan(uint32_t);
            static TextureCube* CreateFromFileFuncVulkan(const std::string& filepath);
            static TextureCube* CreateFromFilesFuncVulkan(const std::string* files);
            static TextureCube* CreateFromVCrossFuncVulkan(const std::string* files, uint32_t mips, TextureParameters params, TextureLoadOptions loadOptions, InputFormat format);

        private:
            std::string m_Name;
            std::string m_Files[MAX_MIPS];
            uint32_t m_Handle {};
            uint32_t m_Width {}, m_Height {}, m_Size {};
            uint32_t m_NumMips {};
            uint8_t* m_Data = nullptr;

            TextureParameters m_Parameters;
            TextureLoadOptions m_LoadOptions;

            TextureFormat m_Format;
            VkFormat m_VKFormat;

            VkImage m_TextureImage {};
            VkImageLayout m_ImageLayout;
            VkDeviceMemory m_TextureImageMemory {};
            VkImageView m_TextureImageView {};
            VkSampler m_TextureSampler {};
            VkDescriptorImageInfo m_Descriptor {};

#ifdef USE_VMA_ALLOCATOR
            VmaAllocation m_Allocation {};
#endif

            bool m_DeleteImage = true;
        };

        class VKTextureDepth : public TextureDepth
        {
        public:
            VKTextureDepth(uint32_t width, uint32_t height);
            ~VKTextureDepth();

            void Bind(uint32_t slot = 0) const override {};
            void Unbind(uint32_t slot = 0) const override {};
            void Resize(uint32_t width, uint32_t height) override;

            inline uint32_t GetWidth() const override
            {
                return m_Width;
            }

            inline uint32_t GetHeight() const override
            {
                return m_Height;
            }

            virtual void* GetHandle() const override
            {
                return (void*)this;
            }

            virtual void* GetImageHande() const override
            {
                return (void*)&m_TextureImage;
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

            TextureFormat GetFormat() const override
            {
                return m_Format;
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
            
            void UpdateDescriptor();
            void TransitionImage(VkImageLayout newLayout, VKCommandBuffer* commandBuffer = nullptr);

            VkImageLayout GetImageLayout() const { return m_ImageLayout; }

            static void MakeDefault();

        protected:
            static TextureDepth* CreateFuncVulkan(uint32_t, uint32_t);
            void Init();

        private:
            std::string m_Name;
            uint32_t m_Handle {};
            uint32_t m_Width, m_Height;
            TextureFormat m_Format;

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
            VKTextureDepthArray(uint32_t width, uint32_t height, uint32_t count);
            ~VKTextureDepthArray();

            void Bind(uint32_t slot = 0) const override {};
            void Unbind(uint32_t slot = 0) const override {};
            void Resize(uint32_t width, uint32_t height, uint32_t count) override;

            inline uint32_t GetWidth() const override
            {
                return m_Width;
            }

            inline uint32_t GetHeight() const override
            {
                return m_Height;
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

            TextureFormat GetFormat() const override
            {
                return m_Format;
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
            
            void* GetDescriptorInfo() const override
            {
                return (void*)GetDescriptor();
            }
            
            void UpdateDescriptor();
            void TransitionImage(VkImageLayout newLayout, VKCommandBuffer* commandBuffer = nullptr);

            void* GetHandleArray(uint32_t index) override;

            uint32_t GetCount() const { return m_Count; }
            VkImageLayout GetImageLayout() const { return m_ImageLayout; }

            static void MakeDefault();

        protected:
            static TextureDepthArray* CreateFuncVulkan(uint32_t, uint32_t, uint32_t);
            void Init() override;

        private:
            std::string m_Name;
            uint32_t m_Handle {};
            uint32_t m_Width, m_Height;
            uint32_t m_Count;

            TextureFormat m_Format;

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
