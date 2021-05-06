#pragma once
#include "Graphics/API/Texture.h"
#include "VK.h"
#include "Graphics/API/GraphicsContext.h"
#include "VKContext.h"

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
            VKTexture2D(VkImage image, VkImageView imageView);
            VKTexture2D();
            ~VKTexture2D();

            void Bind(uint32_t slot = 0) const override {};
            void Unbind(uint32_t slot = 0) const override {};

            virtual void SetData(const void* pixels) override {};

            virtual void* GetHandle() const override
            {
                return (void*)&m_Descriptor;
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

            const TextureParameters& GetTextureParameters() const { return m_Parameters; }

            static void MakeDefault();

        protected:
            static Texture2D* CreateFuncVulkan();
            static Texture2D* CreateFromSourceFuncVulkan(uint32_t, uint32_t, void*, TextureParameters, TextureLoadOptions);
            static Texture2D* CreateFromFileFuncVulkan(const std::string&, const std::string&, TextureParameters, TextureLoadOptions);

        private:
            std::string m_Name;
            std::string m_FileName;
            uint32_t m_Handle {};
            uint32_t m_Width {}, m_Height {};
            uint32_t m_MipLevels = 1;
            uint8_t* m_Data = nullptr;

            TextureParameters m_Parameters;
            TextureLoadOptions m_LoadOptions;

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
                return (void*)&m_Descriptor;
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

            uint32_t GetMipMapLevels() const override
            {
                return m_NumMips;
            }

            const VkDescriptorImageInfo* GetDescriptor() const
            {
                return &m_Descriptor;
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

            virtual void* GetHandle() const override
            {
                return (void*)&m_Descriptor;
            }

            inline const std::string& GetName() const override
            {
                return m_Name;
            }
            inline const std::string& GetFilepath() const override
            {
                return m_Name;
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
            void UpdateDescriptor();

            static void MakeDefault();

        protected:
            static TextureDepth* CreateFuncVulkan(uint32_t, uint32_t);
            void Init();

        private:
            std::string m_Name;
            uint32_t m_Handle {};
            uint32_t m_Width, m_Height;

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

            virtual void* GetHandle() const override
            {
                return (void*)&m_Descriptor;
            }

            inline const std::string& GetName() const override
            {
                return m_Name;
            }
            inline const std::string& GetFilepath() const override
            {
                return m_Name;
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
            void UpdateDescriptor();

            void* GetHandleArray(uint32_t index) override;
            ;

            static void MakeDefault();

        protected:
            static TextureDepthArray* CreateFuncVulkan(uint32_t, uint32_t, uint32_t);
            void Init() override;

        private:
            std::string m_Name;
            uint32_t m_Handle {};
            uint32_t m_Width, m_Height;
            uint32_t m_Count;

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
