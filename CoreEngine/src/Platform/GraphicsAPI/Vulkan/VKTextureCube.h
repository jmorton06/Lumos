#pragma once
#include "Graphics/API/Textures/TextureCube.h"
#include "Dependencies/vulkan/vulkan.h"
#include "Graphics/API/Context.h"
#include "VKContext.h"

#define MAX_MIPS 11

namespace jm
{
	namespace graphics
	{
		class VKTextureCube : public TextureCube
		{
		private:
			String m_Name;
			String m_Files[MAX_MIPS];
			uint m_Handle;
			uint m_Width, m_Height, m_Size;
			uint m_NumMips;
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
			VKTextureCube(uint size);
			VKTextureCube(const String& name, const String& filepath);
			VKTextureCube(const String& name, const String* files);
			VKTextureCube(const String& name, const String* files, uint mips, InputFormat format);
			~VKTextureCube();

			static VkFormat TextureFormatToVK(TextureFormat);

			inline uint GetHandle() const override { return m_Handle; }

			void Bind(uint slot = 0) const override;
			void Unbind(uint slot = 0) const override;

			inline uint GetSize() const override { return m_Size; }
			inline const String& GetName() const override { return m_Name; }
			inline const String& GetFilepath() const override { return m_Files[0]; }

			void CreateTextureSampler();
			void CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling,
				VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image,
				VkDeviceMemory& imageMemory);
			VkImageView createImageView(VkImage image, VkFormat format, uint32_t mipLevels);

			VkDescriptorImageInfo* GetDescriptor() { return &m_Descriptor; }

			void UpdateDescriptor();

			void Load(uint mips);

			VkImage GetImage() const { return m_TextureImage; };
			VkDeviceMemory GetDeviceMemory() const { return m_TextureImageMemory; }
			VkImageView GetImageView() const { return  m_TextureImageView; }
			VkSampler GetSampler() const { return m_TextureSampler; }
		};
	}
}
