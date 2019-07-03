#pragma once
#include "Graphics/API/Textures/TextureDepthArray.h"
#include "VK.h"
#include "Graphics/API/GraphicsContext.h"
#include "VKContext.h"

namespace Lumos
{
	namespace Graphics
	{
		class VKTextureDepthArray : public TextureDepthArray
		{
		public:
			VKTextureDepthArray(u32 width, u32 height, u32 count);
			~VKTextureDepthArray();

			void Bind(u32 slot = 0) const override;
			void Unbind(u32 slot = 0) const override;
			void Resize(u32 width, u32 height, u32 count) override;

			virtual void* GetHandle() const override { return (void*)m_TextureImageView; }

			inline const String& GetName() const override { return m_Name; }
			inline const String& GetFilepath() const override { return m_Name; }

			void CreateTextureSampler();
			void CreateImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling,
			                 vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Image& image,
			                 vk::DeviceMemory& imageMemory);
			void CreateImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags);

			vk::Image GetImage() const { return m_TextureImage; };
			vk::DeviceMemory GetDeviceMemory() const { return m_TextureImageMemory; }
			vk::ImageView GetImageView() const { return  m_TextureImageView; }
			vk::ImageView GetImageView(int index) const { return  m_IndividualImageViews[index]; }
			vk::Sampler GetSampler() const { return m_TextureSampler; }
			vk::DescriptorImageInfo* GetDescriptor() { return &m_Descriptor; }
			void UpdateDescriptor();

		protected:
			void Init() override;

		private:
			String m_Name;
			u32 m_Handle;
			u32 m_Width, m_Height;
			u32 m_Count;

			vk::Image m_TextureImage;
			vk::DeviceMemory m_TextureImageMemory;
			vk::ImageView m_TextureImageView;
			vk::Sampler m_TextureSampler;
			vk::DescriptorImageInfo m_Descriptor;

			std::vector<vk::ImageView> m_IndividualImageViews;
		};
	}
}
