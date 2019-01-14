#pragma once
#include "Graphics/API/Textures/TextureDepth.h"
#include "Dependencies/vulkan/vulkan.h"
#include "Graphics/API/Context.h"
#include "VKContext.h"

namespace jm
{
	namespace graphics
	{
		class VKTextureDepth : public TextureDepth
		{
		private:
			String m_Name;
			uint m_Handle;
			uint m_Width, m_Height;

			VkImage m_TextureImage;
			VkDeviceMemory m_TextureImageMemory;
			VkImageView m_TextureImageView;
			VkSampler m_TextureSampler;
			VkDescriptorImageInfo m_Descriptor;

		public:
			VKTextureDepth(uint width, uint height);
			~VKTextureDepth();

			void Bind(uint slot = 0) const override;
			void Unbind(uint slot = 0) const override;
			void Resize(uint width, uint height) override;

			inline uint GetHandle() const override { return m_Handle; }

			inline const String& GetName() const override { return m_Name; }
			inline const String& GetFilepath() const override { return m_Name; }

			void CreateTextureSampler();
			void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
			                 VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image,
			                 VkDeviceMemory& imageMemory);
			VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

			VkImage GetImage() const { return m_TextureImage; };
			VkDeviceMemory GetDeviceMemory() const { return m_TextureImageMemory; }
			VkImageView GetImageView() const { return  m_TextureImageView; }
			VkSampler GetSampler() const { return m_TextureSampler; }
			VkDescriptorImageInfo* GetDescriptor() { return &m_Descriptor; }
			void UpdateDescriptor();

		protected:
			void Init();
		};
	}
}
