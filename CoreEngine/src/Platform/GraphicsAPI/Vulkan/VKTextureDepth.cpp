#include "JM.h"
#include "VKTextureDepth.h"
#include "VKDevice.h"
#include "Utilities/LoadImage.h"
#include "VKTools.h"

namespace jm
{
	namespace graphics
	{
		VKTextureDepth::VKTextureDepth(uint width, uint height)
			: m_Width(width), m_Height(height), m_TextureSampler(nullptr), m_TextureImageView(nullptr)
		{
			Init();
		}

		VKTextureDepth::~VKTextureDepth()
		{
            if(m_TextureSampler)
                vkDestroySampler(VKDevice::Instance()->GetDevice(), m_TextureSampler, nullptr);

			vkDestroyImageView(VKDevice::Instance()->GetDevice(), m_TextureImageView, nullptr);
			vkDestroyImage(VKDevice::Instance()->GetDevice(), m_TextureImage, nullptr);
			vkFreeMemory(VKDevice::Instance()->GetDevice(), m_TextureImageMemory, nullptr);
		}

		void VKTextureDepth::Bind(uint slot) const
		{
		}

		void VKTextureDepth::Unbind(uint slot) const
		{
		}

		void VKTextureDepth::Init()
		{
			VkFormat depthFormat = VKTools::findDepthFormat();

			CreateImage(m_Width, m_Height, depthFormat, VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_TextureImage,
				m_TextureImageMemory);
            
            if(m_TextureImageView)
                vkDestroyImageView(VKDevice::Instance()->GetDevice(), m_TextureImageView, nullptr);
                
			m_TextureImageView = createImageView(m_TextureImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

			VKTools::transitionImageLayout(m_TextureImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

			CreateTextureSampler();

			UpdateDescriptor();
		}

		void VKTextureDepth::CreateTextureSampler()
		{
            if(m_TextureSampler)
                vkDestroySampler(VKDevice::Instance()->GetDevice(), m_TextureSampler, nullptr);
                
			VkSamplerCreateInfo samplerInfo = {};
			samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerInfo.magFilter = VK_FILTER_LINEAR;
			samplerInfo.minFilter = VK_FILTER_LINEAR;
			samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.anisotropyEnable = VK_TRUE;
			samplerInfo.maxAnisotropy = 16;
			samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
			samplerInfo.unnormalizedCoordinates = VK_FALSE;
			samplerInfo.compareEnable = VK_FALSE;
			samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

			if (vkCreateSampler(VKDevice::Instance()->GetDevice(), &samplerInfo, nullptr, &m_TextureSampler) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create texture sampler!");
			}
		}

		void VKTextureDepth::CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
		                              VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image,
		                              VkDeviceMemory& imageMemory)
		{
			VkImageCreateInfo imageInfo = {};
			imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			imageInfo.extent.width = width;
			imageInfo.extent.height = height;
			imageInfo.extent.depth = 1;
			imageInfo.mipLevels = 1;
			imageInfo.arrayLayers = 1;
			imageInfo.format = format;
			imageInfo.tiling = tiling;
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageInfo.usage = usage;
			imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			if (vkCreateImage(VKDevice::Instance()->GetDevice(), &imageInfo, nullptr, &image) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create image!");
			}

			VkMemoryRequirements memRequirements;
			vkGetImageMemoryRequirements(VKDevice::Instance()->GetDevice(), image, &memRequirements);

			VkMemoryAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = memRequirements.size;
			allocInfo.memoryTypeIndex = VKTools::FindMemoryType(memRequirements.memoryTypeBits, properties);

			if (vkAllocateMemory(VKDevice::Instance()->GetDevice(), &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to allocate image memory!");
			}

			vkBindImageMemory(VKDevice::Instance()->GetDevice(), image, imageMemory, 0);
		}

		VkImageView VKTextureDepth::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
		{
			VkImageViewCreateInfo viewInfo = {};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = image;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = format;
			viewInfo.subresourceRange.aspectMask = aspectFlags;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			VkImageView imageView;
			if (vkCreateImageView(VKDevice::Instance()->GetDevice(), &viewInfo, nullptr, &imageView) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create texture image view!");
			}

			return imageView;
		}

		void VKTextureDepth::UpdateDescriptor()
		{
			m_Descriptor.sampler = m_TextureSampler;
			m_Descriptor.imageView = m_TextureImageView;
			m_Descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}

		void VKTextureDepth::Resize(uint width, uint height)
		{
			m_Width = width;
			m_Height = height;

			Init();
		}
	}
}
