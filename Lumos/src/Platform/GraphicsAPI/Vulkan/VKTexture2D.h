#pragma once
#include "Graphics/API/Textures/Texture2D.h"
#include "Dependencies/vulkan/vulkan.h"
#include "Graphics/API/Context.h"
#include "VKContext.h"

namespace Lumos
{
	namespace graphics
	{
		class VKTexture2D : public Texture2D
		{
		private:
			String m_Name;
			String m_FileName;
			uint m_Handle;
			uint m_Width, m_Height;
			uint m_MipLevels = 1;
			byte* m_Data = nullptr;

			TextureParameters m_Parameters;
			TextureLoadOptions m_LoadOptions;

			VkImage m_TextureImage;
			VkImageLayout m_ImageLayout;
			VkDeviceMemory m_TextureImageMemory;
			VkImageView m_TextureImageView;// = nullptr;
			VkSampler m_TextureSampler;// = nullptr;
			VkDescriptorImageInfo m_Descriptor;

			bool m_DeleteImage = true;
		public:
			VKTexture2D(uint width, uint height, TextureParameters parameters = TextureParameters(), TextureLoadOptions loadOptions = TextureLoadOptions());
			VKTexture2D(uint width, uint height, TextureParameters parameters, TextureLoadOptions loadOptions, void* data);
			VKTexture2D(uint width, uint height, uint color, TextureParameters parameters = TextureParameters(), TextureLoadOptions loadOptions = TextureLoadOptions());
			VKTexture2D(const String& name, const String& filename, TextureParameters parameters = TextureParameters(), TextureLoadOptions loadOptions = TextureLoadOptions());
			VKTexture2D(int width, int height, const void* pixels);
			VKTexture2D(VkImage image, VkImageView imageView);
			VKTexture2D();
			~VKTexture2D();

			void Bind(uint slot = 0) const override;
			void Unbind(uint slot = 0) const override;
			static VkFormat TextureFormatToVK(TextureFormat);

			virtual void SetData(const void* pixels) override {};
			virtual void SetData(uint color) override {};

			virtual uint GetHandle() const override { return m_Handle; }

			inline uint GetWidth() const override { return m_Width; }
			inline uint GetHeight() const override { return m_Height; }

			inline const String& GetName() const override { return m_Name; }
			inline const String& GetFilepath() const override { return m_FileName; }

			void BuildTexture(TextureFormat internalformat, uint width, uint height, bool depth, bool samplerShadow) override;

			void CreateTextureSampler();
			void CreateImage(uint32_t width, uint32_t height,uint32_t mipLevels, VkFormat format, VkImageTiling tiling,
			                 VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image,
			                 VkDeviceMemory& imageMemory);
			VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);

			VkDescriptorImageInfo* GetDescriptor() { return &m_Descriptor; }

			void UpdateDescriptor();

			bool Load();

			VkImage GetImage() const { return m_TextureImage; };
			VkDeviceMemory GetDeviceMemory() const { return m_TextureImageMemory; }
			VkImageView GetImageView() const { return  m_TextureImageView; }
			VkSampler GetSampler() const { return m_TextureSampler; }
		};
	}
}
