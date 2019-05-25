#pragma once
#include "Graphics/API/Textures/TextureCube.h"
#include "VK.h"
#include "Graphics/API/GraphicsContext.h"
#include "VKContext.h"

#define MAX_MIPS 11

namespace lumos
{
	namespace graphics
	{
		class VKTextureCube : public TextureCube
		{
		public:
			VKTextureCube(uint size);
			VKTextureCube(const String& name, const String& filepath);
			VKTextureCube(const String& name, const String* files);
			VKTextureCube(const String& name, const String* files, uint mips, InputFormat format);
			~VKTextureCube();

			static vk::Format TextureFormatToVK(TextureFormat);

			virtual void* GetHandle() const override { return (void*)m_TextureImageView; }

			void Bind(uint slot = 0) const override;
			void Unbind(uint slot = 0) const override;

			inline uint GetSize() const override { return m_Size; }
			inline const String& GetName() const override { return m_Name; }
			inline const String& GetFilepath() const override { return m_Files[0]; }

			void CreateTextureSampler();
			void CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, vk::Format format, vk::ImageTiling tiling,
			vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Image& image,
			vk::DeviceMemory& imageMemory);
			vk::ImageView CreateImageView(vk::Image image, vk::Format format, uint32_t mipLevels);

			vk::DescriptorImageInfo* GetDescriptor() { return &m_Descriptor; }

			void UpdateDescriptor();

			void Load(uint mips);

			vk::Image GetImage() const { return m_TextureImage; };
			vk::DeviceMemory GetDeviceMemory() const { return m_TextureImageMemory; }
			vk::ImageView GetImageView() const { return  m_TextureImageView; }
			vk::Sampler GetSampler() const { return m_TextureSampler; }

		private:
			String m_Name;
			String m_Files[MAX_MIPS];
			uint m_Handle;
			uint m_Width, m_Height, m_Size;
			uint m_NumMips;
			byte* m_Data = nullptr;

			TextureParameters m_Parameters;
			TextureLoadOptions m_LoadOptions;

			vk::Image m_TextureImage;
			vk::ImageLayout m_ImageLayout;
			vk::DeviceMemory m_TextureImageMemory;
			vk::ImageView m_TextureImageView;// = nullptr;
			vk::Sampler m_TextureSampler;// = nullptr;
			vk::DescriptorImageInfo m_Descriptor;

			bool m_DeleteImage = true;
		};
	}
}
