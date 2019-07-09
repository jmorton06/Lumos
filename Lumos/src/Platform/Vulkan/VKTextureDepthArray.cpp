#include "LM.h"
#include "VKTextureDepthArray.h"
#include "VKDevice.h"
#include "Utilities/LoadImage.h"
#include "VKTools.h"

namespace Lumos
{
	namespace Graphics
	{
		VKTextureDepthArray::VKTextureDepthArray(u32 width, u32 height,u32 count)
			: m_Width(width), m_Height(height), m_Count(count)
		{
			Init();
		}

		VKTextureDepthArray::~VKTextureDepthArray()
		{
			vkDestroyImageView(VKDevice::Instance()->GetDevice(), m_TextureImageView, nullptr);
            
#ifdef USE_VMA_ALLOCATOR
            vmaDestroyImage(VKDevice::Instance()->GetAllocator(), m_TextureImage, m_Allocation);
#else
            VKDevice::Instance()->GetDevice().destroyImage(m_TextureImage);
#endif
            vkFreeMemory(VKDevice::Instance()->GetDevice(), m_TextureImageMemory, nullptr);
            vkDestroySampler(VKDevice::Instance()->GetDevice(), m_TextureSampler, nullptr);
            
            for (uint32_t i = 0; i < m_Count; i++)
            {
                vkDestroyImageView(VKDevice::Instance()->GetDevice(), m_IndividualImageViews[i], nullptr);
            }
		}

		void VKTextureDepthArray::Bind(u32 slot) const
		{
		}

		void VKTextureDepthArray::Unbind(u32 slot) const
		{
		}

		void VKTextureDepthArray::Init()
		{
			vk::Format depthFormat = VKTools::FindDepthFormat();

			CreateImage(m_Width, m_Height, depthFormat, vk::ImageTiling::eOptimal,
				vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal, m_TextureImage,
				m_TextureImageMemory);
			CreateImageView(m_TextureImage, depthFormat, vk::ImageAspectFlagBits::eDepth);

			VKTools::TransitionImageLayout(m_TextureImage, depthFormat, vk::ImageLayout::eUndefined,
				vk::ImageLayout::eDepthStencilAttachmentOptimal);

			CreateTextureSampler();

			UpdateDescriptor();
		}

		void VKTextureDepthArray::CreateTextureSampler()
		{
			vk::SamplerCreateInfo samplerInfo = {};
			samplerInfo.magFilter = vk::Filter::eLinear;
			samplerInfo.minFilter = vk::Filter::eLinear;
			samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
			samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
			samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
			samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;
			samplerInfo.maxAnisotropy = 1.0f;
			samplerInfo.borderColor = vk::BorderColor::eFloatOpaqueWhite;
            samplerInfo.mipLodBias = 0.0f;
            samplerInfo.minLod = 0.0f;
            samplerInfo.maxLod = 1.0f;
			//samplerInfo.unnormalizedCoordinates = VK_FALSE;
			//samplerInfo.compareEnable = VK_FALSE;
			//samplerInfo.compareOp = vk::CompareOp::eAlways;

			m_TextureSampler = VKDevice::Instance()->GetDevice().createSampler(samplerInfo);
		}

		void VKTextureDepthArray::CreateImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling,
		                              vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Image& image,
		                              vk::DeviceMemory& imageMemory)
		{
			vk::ImageCreateInfo imageInfo = {};
			imageInfo.imageType = vk::ImageType::e2D;
			imageInfo.extent.width = width;
			imageInfo.extent.height = height;
			imageInfo.extent.depth = 1;
			imageInfo.mipLevels = 1;
			imageInfo.arrayLayers = m_Count;
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
#endif
			vk::MemoryRequirements memRequirements;
			VKDevice::Instance()->GetDevice().getImageMemoryRequirements(image, &memRequirements);

			vk::MemoryAllocateInfo allocInfo = {};
			allocInfo.allocationSize = memRequirements.size;
			allocInfo.memoryTypeIndex = VKTools::FindMemoryType(memRequirements.memoryTypeBits, properties);

			imageMemory = VKDevice::Instance()->GetDevice().allocateMemory(allocInfo);
			VKDevice::Instance()->GetDevice().bindImageMemory(image, imageMemory, 0);
		}

		void VKTextureDepthArray::CreateImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags)
		{
			vk::ImageViewCreateInfo viewInfo = {};
			viewInfo.image = image;
			viewInfo.viewType = vk::ImageViewType::e2DArray;
			viewInfo.format = format;
			viewInfo.subresourceRange.aspectMask = aspectFlags;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = m_Count;

			m_TextureImageView = VKDevice::Instance()->GetDevice().createImageView(viewInfo);

			for (uint32_t i = 0; i < m_Count; i++)
			{
				vk::ImageViewCreateInfo viewInfo = {};
				viewInfo.image = image;
				viewInfo.viewType = vk::ImageViewType::e2D;
				viewInfo.format = format;
				viewInfo.subresourceRange.aspectMask = aspectFlags;
				viewInfo.subresourceRange.baseMipLevel = 0;
				viewInfo.subresourceRange.levelCount = 1;
				viewInfo.subresourceRange.baseArrayLayer = i;
				viewInfo.subresourceRange.layerCount = 1;
				
				vk::ImageView imageView = VKDevice::Instance()->GetDevice().createImageView(viewInfo);
				m_IndividualImageViews.push_back(imageView);
			}
		}

		void VKTextureDepthArray::UpdateDescriptor()
		{
			m_Descriptor.sampler = m_TextureSampler;
			m_Descriptor.imageView = m_TextureImageView;
			m_Descriptor.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		}

		void VKTextureDepthArray::Resize(u32 width, u32 height, u32 count)
		{
			m_Width = width;
			m_Height = height;
			m_Count = count;
			Init();
		}
	}
}
