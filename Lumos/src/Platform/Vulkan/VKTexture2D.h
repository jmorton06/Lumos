#pragma once
#include "Graphics/API/Textures/Texture2D.h"
#include "VK.h"
#include "Graphics/API/GraphicsContext.h"
#include "VKContext.h"

namespace lumos
{
	namespace graphics
	{
		class VKTexture2D : public Texture2D
		{
		public:
			VKTexture2D(uint width, uint height, TextureParameters parameters = TextureParameters(), TextureLoadOptions loadOptions = TextureLoadOptions());
			VKTexture2D(uint width, uint height, TextureParameters parameters, TextureLoadOptions loadOptions, void* data);
			VKTexture2D(uint width, uint height, uint color, TextureParameters parameters = TextureParameters(), TextureLoadOptions loadOptions = TextureLoadOptions());
			VKTexture2D(const String& name, const String& filename, TextureParameters parameters = TextureParameters(), TextureLoadOptions loadOptions = TextureLoadOptions());
			VKTexture2D(int width, int height, void* pixels);
            VKTexture2D(vk::Image image, vk::ImageView imageView);
			VKTexture2D();
			~VKTexture2D();

			void Bind(uint slot = 0) const override;
			void Unbind(uint slot = 0) const override;
            static vk::Format TextureFormatToVK(TextureFormat);

			virtual void SetData(const void* pixels) override {};
			virtual void SetData(uint color) override {};

			virtual void* GetHandle() const override { return (void*)&m_Descriptor; }

			inline uint GetWidth() const override { return m_Width; }
			inline uint GetHeight() const override { return m_Height; }

			inline const String& GetName() const override { return m_Name; }
			inline const String& GetFilepath() const override { return m_FileName; }

			void BuildTexture(TextureFormat internalformat, uint width, uint height, bool depth, bool samplerShadow) override;

			void CreateTextureSampler();
            void CreateImage(uint32_t width, uint32_t height,uint32_t mipLevels, vk::Format format, vk::ImageTiling tiling,
                             vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Image& image,
                             vk::DeviceMemory& imageMemory);
            vk::ImageView CreateImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels);

            vk::DescriptorImageInfo* GetDescriptor() { return &m_Descriptor; }

			void UpdateDescriptor();

			bool Load();

            vk::Image GetImage() const { return m_TextureImage; };
            vk::DeviceMemory GetDeviceMemory() const { return m_TextureImageMemory; }
            vk::ImageView GetImageView() const { return  m_TextureImageView; }
            vk::Sampler GetSampler() const { return m_TextureSampler; }
            
        private:
            String m_Name;
            String m_FileName;
            uint m_Handle;
            uint m_Width, m_Height;
            uint m_MipLevels = 1;
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
