#pragma once
#include "Graphics/API/Textures/TextureDepth.h"
#include "VK.h"
#include "Graphics/API/GraphicsContext.h"
#include "VKContext.h"

namespace lumos
{
	namespace graphics
	{
		class VKTextureDepth : public TextureDepth
		{
		public:
			VKTextureDepth(uint width, uint height);
			~VKTextureDepth();

			void Bind(uint slot = 0) const override;
			void Unbind(uint slot = 0) const override;
			void Resize(uint width, uint height) override;

			virtual void* GetHandle() const override { return (void*)m_TextureImageView; }

			inline const String& GetName() const override { return m_Name; }
			inline const String& GetFilepath() const override { return m_Name; }

			void CreateTextureSampler();
			void CreateImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling,
			                 vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Image& image,
			                 vk::DeviceMemory& imageMemory);
			vk::ImageView CreateImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags);
			vk::Image GetImage() const { return m_TextureImage; };
			vk::DeviceMemory GetDeviceMemory() const { return m_TextureImageMemory; }
			vk::ImageView GetImageView() const { return  m_TextureImageView; }
			vk::Sampler GetSampler() const { return m_TextureSampler; }
			vk::DescriptorImageInfo* GetDescriptor() { return &m_Descriptor; }
			void UpdateDescriptor();

		protected:
			void Init();

		private:
			String m_Name;
			uint m_Handle;
			uint m_Width, m_Height;

			vk::Image m_TextureImage;
			vk::DeviceMemory m_TextureImageMemory;
			vk::ImageView m_TextureImageView;
			vk::Sampler m_TextureSampler;
			vk::DescriptorImageInfo m_Descriptor;
		};
	}
}
