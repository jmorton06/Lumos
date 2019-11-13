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
			VKTexture2D(u32 width, u32 height, void* data, TextureParameters parameters = TextureParameters(), TextureLoadOptions loadOptions = TextureLoadOptions());
			VKTexture2D(const String& name, const String& filename, TextureParameters parameters = TextureParameters(), TextureLoadOptions loadOptions = TextureLoadOptions());
            VKTexture2D(VkImage image, VkImageView imageView);
			VKTexture2D();
			~VKTexture2D();

			void Bind(u32 slot = 0) const override;
			void Unbind(u32 slot = 0) const override;

			virtual void SetData(const void* pixels) override {};

			virtual void* GetHandle() const override { return (void*)&m_Descriptor; }

			inline u32 GetWidth() const override { return m_Width; }
			inline u32 GetHeight() const override { return m_Height; }

			inline const String& GetName() const override { return m_Name; }
			inline const String& GetFilepath() const override { return m_FileName; }

			void BuildTexture(TextureFormat internalformat, u32 width, u32 height, bool depth, bool samplerShadow) override;

			void CreateTextureSampler();
            void CreateImage(uint32_t width, uint32_t height,uint32_t mipLevels, VkFormat format, VkImageTiling tiling,
                             VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image,
                             VkDeviceMemory& imageMemory);
            VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);

            const VkDescriptorImageInfo* GetDescriptor() const { return &m_Descriptor; }

			void UpdateDescriptor();

			bool Load();

            VkImage GetImage() const { return m_TextureImage; };
            VkDeviceMemory GetDeviceMemory() const { return m_TextureImageMemory; }
            VkImageView GetImageView() const { return  m_TextureImageView; }
            VkSampler GetSampler() const { return m_TextureSampler; }
            
            static void MakeDefault();
        protected:
            static Texture2D* CreateFuncVulkan();
			static Texture2D* CreateFromSourceFuncVulkan(u32, u32, void*, TextureParameters, TextureLoadOptions);
			static Texture2D* CreateFromFileFuncVulkan(const String&, const String&, TextureParameters, TextureLoadOptions);
            
        private:
            String m_Name;
            String m_FileName;
            u32 m_Handle;
            u32 m_Width, m_Height;
            u32 m_MipLevels = 1;
            u8* m_Data = nullptr;
            
            TextureParameters m_Parameters;
            TextureLoadOptions m_LoadOptions;
            
            VkImage m_TextureImage;
            VkImageLayout m_ImageLayout;
            VkDeviceMemory m_TextureImageMemory;
            VkImageView m_TextureImageView;
            VkSampler m_TextureSampler;
            VkDescriptorImageInfo m_Descriptor;
            
#ifdef USE_VMA_ALLOCATOR
            VmaAllocation m_Allocation;
#endif
            
            bool m_DeleteImage = true;
		};

		class VKTextureCube : public TextureCube
		{
		public:
			VKTextureCube(u32 size);
			VKTextureCube(const String& filepath);
			VKTextureCube(const String* files);
			VKTextureCube(const String* files, u32 mips, InputFormat format);
			~VKTextureCube();

			virtual void* GetHandle() const override { return (void*)&m_Descriptor; }

			void Bind(u32 slot = 0) const override;
			void Unbind(u32 slot = 0) const override;

			inline u32 GetSize() const override { return m_Size; }
			inline const String& GetName() const override { return m_Name; }
			inline const String& GetFilepath() const override { return m_Files[0]; }

			void CreateTextureSampler();
			void CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling,
				VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image,
				VkDeviceMemory& imageMemory);
			VkImageView CreateImageView(VkImage image, VkFormat format, uint32_t mipLevels);

			const VkDescriptorImageInfo* GetDescriptor() const { return &m_Descriptor; }

			void UpdateDescriptor();

			void Load(u32 mips);

			VkImage GetImage() const { return m_TextureImage; };
			VkDeviceMemory GetDeviceMemory() const { return m_TextureImageMemory; }
			VkImageView GetImageView() const { return  m_TextureImageView; }
			VkSampler GetSampler() const { return m_TextureSampler; }

            static void MakeDefault();
        protected:
			static TextureCube* CreateFuncVulkan(u32);
			static TextureCube* CreateFromFileFuncVulkan(const String& filepath);
			static TextureCube* CreateFromFilesFuncVulkan(const String* files);
			static TextureCube* CreateFromVCrossFuncVulkan(const String* files, u32 mips, InputFormat format);
            
		private:
			String m_Name;
			String m_Files[MAX_MIPS];
			u32 m_Handle;
			u32 m_Width, m_Height, m_Size;
			u32 m_NumMips;
			u8* m_Data = nullptr;

			TextureParameters m_Parameters;
			TextureLoadOptions m_LoadOptions;

			VkImage m_TextureImage;
			VkImageLayout m_ImageLayout;
			VkDeviceMemory m_TextureImageMemory;
			VkImageView m_TextureImageView;
			VkSampler m_TextureSampler;
			VkDescriptorImageInfo m_Descriptor;

#ifdef USE_VMA_ALLOCATOR
			VmaAllocation m_Allocation;
#endif

			bool m_DeleteImage = true;
		};

		class VKTextureDepth : public TextureDepth
		{
		public:
			VKTextureDepth(u32 width, u32 height);
			~VKTextureDepth();

			void Bind(u32 slot = 0) const override;
			void Unbind(u32 slot = 0) const override;
			void Resize(u32 width, u32 height) override;

			virtual void* GetHandle() const override { return (void*)&m_Descriptor; }

			inline const String& GetName() const override { return m_Name; }
			inline const String& GetFilepath() const override { return m_Name; }

			void CreateTextureSampler();
			void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
				VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image,
				VkDeviceMemory& imageMemory);
			VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
			VkImage GetImage() const { return m_TextureImage; };
			VkDeviceMemory GetDeviceMemory() const { return m_TextureImageMemory; }
			VkImageView GetImageView() const { return  m_TextureImageView; }
			VkSampler GetSampler() const { return m_TextureSampler; }
			const VkDescriptorImageInfo* GetDescriptor() const { return &m_Descriptor; }
			void UpdateDescriptor();

            static void MakeDefault();
        protected:
            static TextureDepth* CreateFuncVulkan(u32, u32);
			void Init();

		private:
			String m_Name;
			u32 m_Handle;
			u32 m_Width, m_Height;

			VkImage m_TextureImage;
			VkDeviceMemory m_TextureImageMemory;
			VkImageView m_TextureImageView;
			VkSampler m_TextureSampler;
			VkDescriptorImageInfo m_Descriptor;

#ifdef USE_VMA_ALLOCATOR
			VmaAllocation m_Allocation;
#endif
		};

		class VKTextureDepthArray : public TextureDepthArray
		{
		public:
			VKTextureDepthArray(u32 width, u32 height, u32 count);
			~VKTextureDepthArray();

			void Bind(u32 slot = 0) const override;
			void Unbind(u32 slot = 0) const override;
			void Resize(u32 width, u32 height, u32 count) override;

			virtual void* GetHandle() const override { return (void*)&m_Descriptor; }

			inline const String& GetName() const override { return m_Name; }
			inline const String& GetFilepath() const override { return m_Name; }

			void CreateTextureSampler();
			void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
				VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image,
				VkDeviceMemory& imageMemory);
			void CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

			VkImage GetImage() const { return m_TextureImage; };
			VkDeviceMemory GetDeviceMemory() const { return m_TextureImageMemory; }
			VkImageView GetImageView() const { return  m_TextureImageView; }
			VkImageView GetImageView(int index) const { return  m_IndividualImageViews[index]; }
			VkSampler GetSampler() const { return m_TextureSampler; }
			const VkDescriptorImageInfo* GetDescriptor() const { return &m_Descriptor; }
			void UpdateDescriptor();

            static void MakeDefault();
        protected:
            static TextureDepthArray* CreateFuncVulkan(u32, u32, u32);
			void Init() override;

		private:
			String m_Name;
			u32 m_Handle;
			u32 m_Width, m_Height;
			u32 m_Count;

			VkImage m_TextureImage;
			VkDeviceMemory m_TextureImageMemory;
			VkImageView m_TextureImageView;
			VkSampler m_TextureSampler;
			VkDescriptorImageInfo m_Descriptor;

			std::vector<VkImageView> m_IndividualImageViews;

#ifdef USE_VMA_ALLOCATOR
			VmaAllocation m_Allocation;
#endif
		};
	}
}
