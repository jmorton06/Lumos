#include "LM.h"
#include "VKTextureDepth.h"
#include "VKDevice.h"
#include "Utilities/LoadImage.h"
#include "VKTools.h"

namespace Lumos
{
	namespace Graphics
	{
		VKTextureDepth::VKTextureDepth(u32 width, u32 height)
			: m_Width(width), m_Height(height), m_TextureSampler(nullptr), m_TextureImageView(nullptr)
		{
			Init();
		}

		VKTextureDepth::~VKTextureDepth()
		{
			if (m_TextureSampler)
				VKDevice::Instance()->GetDevice().destroySampler(m_TextureSampler);
	
			VKDevice::Instance()->GetDevice().destroyImageView(m_TextureImageView);
#ifdef USE_VMA_ALLOCATOR
            vmaDestroyImage(VKDevice::Instance()->GetAllocator(), m_TextureImage, m_Allocation);
#else
            VKDevice::Instance()->GetDevice().destroyImage(m_TextureImage);
            VKDevice::Instance()->GetDevice().freeMemory(m_TextureImageMemory);
#endif
		}

		void VKTextureDepth::Bind(u32 slot) const
		{
		}

		void VKTextureDepth::Unbind(u32 slot) const
		{
		}

		void VKTextureDepth::Init()
		{
			vk::Format depthFormat = VKTools::FindDepthFormat();

			CreateImage(m_Width, m_Height, depthFormat, vk::ImageTiling::eOptimal,
				vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal, m_TextureImage,
				m_TextureImageMemory);
                
			m_TextureImageView = CreateImageView(m_TextureImage, depthFormat, vk::ImageAspectFlagBits::eDepth);

			//VKTools::TransitionImageLayout(m_TextureImage, depthFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal);

			CreateTextureSampler();

			UpdateDescriptor();
		}

		void VKTextureDepth::CreateTextureSampler()
		{
			vk::SamplerCreateInfo samplerInfo = {};
			samplerInfo.magFilter = vk::Filter::eLinear;
			samplerInfo.minFilter = vk::Filter::eLinear;
			samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
			samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
			samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
			samplerInfo.anisotropyEnable = VK_TRUE;
			samplerInfo.maxAnisotropy = VKDevice::Instance()->GetGPUProperties().limits.maxSamplerAnisotropy;
			samplerInfo.borderColor = vk::BorderColor::eFloatOpaqueBlack;
			samplerInfo.unnormalizedCoordinates = VK_FALSE;
			samplerInfo.compareEnable = VK_FALSE;
			samplerInfo.compareOp = vk::CompareOp::eAlways;
			samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;

			m_TextureSampler = VKDevice::Instance()->GetDevice().createSampler(samplerInfo);
		}

		void VKTextureDepth::CreateImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling,
		                              vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Image& image,
		                              vk::DeviceMemory& imageMemory)
		{
			vk::ImageCreateInfo imageInfo = {};
			imageInfo.imageType = vk::ImageType::e2D;
			imageInfo.extent.width = width;
			imageInfo.extent.height = height;
			imageInfo.extent.depth = 1;
			imageInfo.mipLevels = 1;
			imageInfo.arrayLayers = 1;
			imageInfo.format = format;
			imageInfo.tiling = tiling;
			imageInfo.initialLayout = vk::ImageLayout::eUndefined;
			imageInfo.usage = usage;
			imageInfo.samples = vk::SampleCountFlagBits::e1;
			imageInfo.sharingMode = vk::SharingMode::eExclusive;

#ifdef USE_VMA_ALLOCATOR
            VmaAllocationCreateInfo allocInfovma;
            allocInfovma.flags = 0;
            allocInfovma.usage = VMA_MEMORY_USAGE_GPU_ONLY;
            allocInfovma.requiredFlags = 0;
            allocInfovma.preferredFlags = 0;
            allocInfovma.memoryTypeBits = 0;
            allocInfovma.pool = nullptr;
            allocInfovma.pUserData = nullptr;
            vmaCreateImage(VKDevice::Instance()->GetAllocator(), reinterpret_cast<VkImageCreateInfo*>(&imageInfo), &allocInfovma, reinterpret_cast<VkImage*>(&image), &m_Allocation, nullptr);
#else
            image = VKDevice::Instance()->GetDevice().createImage(imageInfo);

			vk::MemoryRequirements memRequirements;
			VKDevice::Instance()->GetDevice().getImageMemoryRequirements(image, &memRequirements);

			vk::MemoryAllocateInfo allocInfo = {};
			allocInfo.allocationSize = memRequirements.size;
			allocInfo.memoryTypeIndex = VKTools::FindMemoryType(memRequirements.memoryTypeBits, properties);

			imageMemory = VKDevice::Instance()->GetDevice().allocateMemory(allocInfo);
			VKDevice::Instance()->GetDevice().bindImageMemory(image, imageMemory, 0);
#endif
		}

		vk::ImageView VKTextureDepth::CreateImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags)
		{
			vk::ImageViewCreateInfo viewInfo = {};
			viewInfo.image = image;
			viewInfo.viewType = vk::ImageViewType::e2D;
			viewInfo.format = format;
			viewInfo.subresourceRange.aspectMask = aspectFlags;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			vk::ImageView imageView = VKDevice::Instance()->GetDevice().createImageView(viewInfo);

			return imageView;
		}

		void VKTextureDepth::UpdateDescriptor()
		{
			m_Descriptor.sampler = m_TextureSampler;
			m_Descriptor.imageView = m_TextureImageView;
			m_Descriptor.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		}

		void VKTextureDepth::Resize(u32 width, u32 height)
		{
			m_Width = width;
			m_Height = height;

            if (m_TextureSampler)
                VKDevice::Instance()->GetDevice().destroySampler(m_TextureSampler);
            
            VKDevice::Instance()->GetDevice().destroyImageView(m_TextureImageView);
#ifdef USE_VMA_ALLOCATOR
            vmaDestroyImage(VKDevice::Instance()->GetAllocator(), m_TextureImage, m_Allocation);
#else
            VKDevice::Instance()->GetDevice().destroyImage(m_TextureImage);
            VKDevice::Instance()->GetDevice().freeMemory(m_TextureImageMemory);
#endif
            
			Init();
		}
	}
}
