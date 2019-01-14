#include "JM.h"
#include "VKTextureDepthArray.h"
#include "VKDevice.h"
#include "Utilities/LoadImage.h"
#include "VKTools.h"
#include "VKInitialisers.h"

namespace jm
{
	namespace graphics
	{
		VKTextureDepthArray::VKTextureDepthArray(uint width, uint height,uint count)
			: m_Width(width), m_Height(height), m_Count(count)
		{
			Init();
		}

		VKTextureDepthArray::~VKTextureDepthArray()
		{
			vkDestroyImageView(VKDevice::Instance()->GetDevice(), m_TextureImageView, nullptr);
			vkDestroyImage(VKDevice::Instance()->GetDevice(), m_TextureImage, nullptr);
			vkFreeMemory(VKDevice::Instance()->GetDevice(), m_TextureImageMemory, nullptr);
		}

		void VKTextureDepthArray::Bind(uint slot) const
		{
		}

		void VKTextureDepthArray::Unbind(uint slot) const
		{
		}

		void VKTextureDepthArray::Init()
		{
			VkFormat depthFormat = VKTools::findDepthFormat();

			CreateImage(m_Width, m_Height, depthFormat, VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_TextureImage,
				m_TextureImageMemory);
			createImageView(m_TextureImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

			VKTools::transitionImageLayout(m_TextureImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

			CreateTextureSampler();

			UpdateDescriptor();
		}

		void VKTextureDepthArray::CreateTextureSampler()
		{
			VkSamplerCreateInfo sampler = initializers::samplerCreateInfo();
			sampler.magFilter = VK_FILTER_LINEAR;
			sampler.minFilter = VK_FILTER_LINEAR;
			sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			sampler.addressModeV = sampler.addressModeU;
			sampler.addressModeW = sampler.addressModeU;
			sampler.mipLodBias = 0.0f;
			sampler.maxAnisotropy = 1.0f;
			sampler.minLod = 0.0f;
			sampler.maxLod = 1.0f;
			sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

			if (vkCreateSampler(VKDevice::Instance()->GetDevice(), &sampler, nullptr, &m_TextureSampler) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create texture sampler!");
			}
		}

		void VKTextureDepthArray::CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
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
			imageInfo.arrayLayers = m_Count;
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

		void VKTextureDepthArray::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
		{
			VkImageViewCreateInfo viewInfo = {};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = image;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
			viewInfo.format = format;
			viewInfo.subresourceRange.aspectMask = aspectFlags;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = m_Count;

			if (vkCreateImageView(VKDevice::Instance()->GetDevice(), &viewInfo, nullptr, &m_TextureImageView) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create texture image view!");
			}

			for (uint32_t i = 0; i < m_Count; i++)
			{
				VkImageViewCreateInfo viewInfo = {};
				viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
				viewInfo.format = format;
				viewInfo.subresourceRange = {};
				viewInfo.subresourceRange.aspectMask = aspectFlags;
				viewInfo.subresourceRange.baseMipLevel = 0;
				viewInfo.subresourceRange.levelCount = 1;
				viewInfo.subresourceRange.baseArrayLayer = i;
				viewInfo.subresourceRange.layerCount = 1;
				viewInfo.image = image;

				VkImageView imageView;
				vkCreateImageView(VKDevice::Instance()->GetDevice(), &viewInfo, nullptr, &imageView);
				m_IndividualImageViews.push_back(imageView);
			}
		}

		void VKTextureDepthArray::UpdateDescriptor()
		{
			m_Descriptor.sampler = m_TextureSampler;
			m_Descriptor.imageView = m_TextureImageView;
			m_Descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}

		void VKTextureDepthArray::Resize(uint width, uint height, uint count)
		{
			m_Width = width;
			m_Height = height;
			m_Count = count;
			Init();
		}
	}
}
