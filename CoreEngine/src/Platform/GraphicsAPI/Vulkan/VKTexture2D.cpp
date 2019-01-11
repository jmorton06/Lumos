#include "JM.h"
#include "VKTexture2D.h"
#include "VKDevice.h"
#include "Utilities/LoadImage.h"
#include "VKTools.h"
#include "VKCommandBuffer.h"

#include "Maths/MathsUtilities.h"

#include <cmath>

namespace jm
{
	namespace graphics
	{
		VKTexture2D::VKTexture2D(uint width, uint height, TextureParameters parameters, TextureLoadOptions loadOptions)
			: m_FileName("NULL"), m_TextureSampler(NULL), m_TextureImageView(NULL)
		{
			m_Width = width;
			m_Height = height;
			m_Parameters = parameters;
			m_LoadOptions = loadOptions;
			Load();

			UpdateDescriptor();
		}

		VKTexture2D::VKTexture2D(uint width, uint height, TextureParameters parameters, TextureLoadOptions loadOptions,
			void* data)
			: m_FileName("NULL"), m_TextureSampler(NULL), m_TextureImageView(NULL)
		{
			m_Width = width;
			m_Height = height;
			m_Parameters = parameters;
			m_LoadOptions = loadOptions;
			m_Data = static_cast<byte*>(data);
			Load();

			m_TextureImageView = createImageView(m_TextureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, m_MipLevels);

			CreateTextureSampler();

			UpdateDescriptor();
		}

		VKTexture2D::VKTexture2D(uint width, uint height, uint color, TextureParameters parameters,
			TextureLoadOptions loadOptions)
			: m_FileName("NULL"), m_TextureSampler(NULL), m_TextureImageView(NULL)
		{
			m_Width = width;
			m_Height = height;
			m_Parameters = parameters;
			m_LoadOptions = loadOptions;
			Load();

			UpdateDescriptor();
		}

		VKTexture2D::VKTexture2D(const String& name, const String& filename, TextureParameters parameters,
			TextureLoadOptions loadOptions)
			: m_FileName(filename), m_TextureSampler(NULL), m_TextureImageView(NULL)
		{
			m_Parameters = parameters;
			m_LoadOptions = loadOptions;
			m_DeleteImage = Load();

			if(!m_DeleteImage)
				return;

			m_TextureImageView = createImageView(m_TextureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, m_MipLevels);

			CreateTextureSampler();
			UpdateDescriptor();
		}

		VKTexture2D::VKTexture2D(int width, int height, const void* pixels)
			: m_FileName("NULL"), m_TextureSampler(NULL), m_TextureImageView(NULL)
		{
			m_Width = width;
			m_Height = height;
			m_Parameters = TextureParameters();
			m_LoadOptions = TextureLoadOptions();
			Load();

			UpdateDescriptor();
		}

		VKTexture2D::VKTexture2D(VkImage image, VkImageView imageView) : m_TextureImage(image), m_TextureImageView(imageView), m_TextureSampler(NULL)
		{
			m_DeleteImage = false;
		}

		VKTexture2D::VKTexture2D()
			: m_FileName("NULL"), m_TextureSampler(NULL), m_TextureImageView(NULL)
		{
			m_Width = 0;
			m_Height = 0;
			m_Parameters = TextureParameters();
			m_LoadOptions = TextureLoadOptions();
			m_DeleteImage = false;
			CreateTextureSampler();
			UpdateDescriptor();
		}

		VKTexture2D::~VKTexture2D()
		{
			if(m_TextureSampler)
				vkDestroySampler(VKDevice::Instance()->GetDevice(), m_TextureSampler, nullptr);

			if (m_TextureImageView)
				vkDestroyImageView(VKDevice::Instance()->GetDevice(), m_TextureImageView, nullptr);

			if (m_DeleteImage)
			{
				vkDestroyImage(VKDevice::Instance()->GetDevice(), m_TextureImage, nullptr);
				vkFreeMemory(VKDevice::Instance()->GetDevice(), m_TextureImageMemory, nullptr);
			}
		}

		void VKTexture2D::Bind(uint slot) const
		{
		}

		void VKTexture2D::Unbind(uint slot) const
		{
		}

		VkFormat VKTexture2D::TextureFormatToVK(const TextureFormat format)
		{
			switch (format)
			{
			case TextureFormat::RGBA:				return VK_FORMAT_R8G8B8A8_UNORM;
			case TextureFormat::RGB:				return VK_FORMAT_R8G8B8_UNORM;
			case TextureFormat::R8:				    return VK_FORMAT_R8_UNORM;
			case TextureFormat::RG8:				return VK_FORMAT_R8G8_UNORM;
			case TextureFormat::RGB8:				return VK_FORMAT_R8G8B8_UNORM;
			case TextureFormat::RGBA8:				return VK_FORMAT_R8G8B8A8_UNORM;
			case TextureFormat::LUMINANCE:			return VK_FORMAT_R8G8B8A8_UNORM;
			case TextureFormat::LUMINANCE_ALPHA:	return VK_FORMAT_R8G8B8A8_UNORM;
			case TextureFormat::RGB16: 				return VK_FORMAT_R16G16B16A16_SFLOAT;
			default: JM_CORE_ERROR("[Texture] Unsupported image bit-depth!");  return VK_FORMAT_R8G8B8A8_UNORM;
			}
		}

		VkSamplerAddressMode TextureWrapToVK(const TextureWrap wrap)
		{
			switch (wrap)
			{
			case TextureWrap::CLAMP:			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			case TextureWrap::CLAMP_TO_BORDER:	return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			case TextureWrap::CLAMP_TO_EDGE:	return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			case TextureWrap::REPEAT:			return VK_SAMPLER_ADDRESS_MODE_REPEAT;
			case TextureWrap::MIRRORED_REPEAT:	return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			default: JM_CORE_ERROR("[Texture] Unsupported wrap type!");  return VkSamplerAddressMode();
			}
		}

		void VKTexture2D::BuildTexture(TextureFormat internalformat, uint width, uint height, bool depth, bool samplerShadow)
		{
			m_Width = width;
			m_Height = height;
			m_Handle = 0;
			m_DeleteImage = false;
			m_MipLevels = 1;
			CreateImage(m_Width, m_Height, m_MipLevels,TextureFormatToVK(internalformat), VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_TextureImage,
				m_TextureImageMemory);

			VKCommandBuffer cmdBuffer;
			cmdBuffer.Init(true);

			//VKTools::SetImageLayout(m_TextureImage, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, NULL, &cmdBuffer, true);

			m_TextureImageView = createImageView(m_TextureImage, TextureFormatToVK(internalformat), VK_IMAGE_ASPECT_COLOR_BIT, m_MipLevels);

			UpdateDescriptor();
		}

		void VKTexture2D::CreateTextureSampler()
		{
			VkSamplerCreateInfo samplerInfo = {};
			samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerInfo.magFilter = m_Parameters.filter == TextureFilter::LINEAR ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
			samplerInfo.minFilter = m_Parameters.filter == TextureFilter::LINEAR ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
			samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.anisotropyEnable = VK_TRUE;
			samplerInfo.maxAnisotropy = VKDevice::Instance()->GetGPUProperties().limits.maxSamplerAnisotropy;
			samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
			samplerInfo.unnormalizedCoordinates = VK_FALSE;
			samplerInfo.compareEnable = VK_FALSE;
			samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			samplerInfo.minLod = 0;
			samplerInfo.maxLod = static_cast<float>(m_MipLevels);
			samplerInfo.mipLodBias = 0;
			samplerInfo.addressModeU = TextureWrapToVK(m_Parameters.wrap);
			samplerInfo.addressModeV = TextureWrapToVK(m_Parameters.wrap);
			samplerInfo.addressModeW = TextureWrapToVK(m_Parameters.wrap);

			if (vkCreateSampler(VKDevice::Instance()->GetDevice(), &samplerInfo, nullptr, &m_TextureSampler) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create texture sampler!");
			}
		}

		void VKTexture2D::CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling,
		                              VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image,
		                              VkDeviceMemory& imageMemory)
		{
			VkImageCreateInfo imageInfo = {};
			imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			imageInfo.extent.width = width;
			imageInfo.extent.height = height;
			imageInfo.extent.depth = 1;
			imageInfo.mipLevels = mipLevels;
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

		VkImageView VKTexture2D::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels)
		{
			VkImageViewCreateInfo viewInfo = {};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = image;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = format;
			viewInfo.subresourceRange.aspectMask = aspectFlags;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = mipLevels;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			VkImageView imageView;
			if (vkCreateImageView(VKDevice::Instance()->GetDevice(), &viewInfo, nullptr, &imageView) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create texture image view!");
			}

			return imageView;
		}

		void VKTexture2D::UpdateDescriptor()
		{
			m_Descriptor.sampler = m_TextureSampler;
			m_Descriptor.imageView = m_TextureImageView;
			m_Descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}

		void GenerateMipmaps(VkImage image, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) 
		{
			VkCommandBuffer commandBuffer = VKTools::beginSingleTimeCommands();

			VkImageMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.image = image;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;
			barrier.subresourceRange.levelCount = 1;

			int32_t mipWidth = texWidth;
			int32_t mipHeight = texHeight;

			for (uint32_t i = 1; i < mipLevels; i++) {
				barrier.subresourceRange.baseMipLevel = i - 1;
				barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

				vkCmdPipelineBarrier(commandBuffer,
					VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
					0, nullptr,
					0, nullptr,
					1, &barrier);

				VkImageBlit blit = {};
				blit.srcOffsets[0] = { 0, 0, 0 };
				blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
				blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				blit.srcSubresource.mipLevel = i - 1;
				blit.srcSubresource.baseArrayLayer = 0;
				blit.srcSubresource.layerCount = 1;
				blit.dstOffsets[0] = { 0, 0, 0 };
				blit.dstOffsets[1] = { mipWidth / 2, mipHeight / 2, 1 };
				blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				blit.dstSubresource.mipLevel = i;
				blit.dstSubresource.baseArrayLayer = 0;
				blit.dstSubresource.layerCount = 1;

				vkCmdBlitImage(commandBuffer,
					image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1, &blit,
					VK_FILTER_LINEAR);

				barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				vkCmdPipelineBarrier(commandBuffer,
					VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
					0, nullptr,
					0, nullptr,
					1, &barrier);

				if (mipWidth > 1) mipWidth /= 2;
				if (mipHeight > 1) mipHeight /= 2;
			}

			barrier.subresourceRange.baseMipLevel = mipLevels - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			VKTools::endSingleTimeCommands(commandBuffer);
		}


		bool VKTexture2D::Load()
		{
			uint texWidth, texHeight, texChannels;
			byte* pixels;

			if (m_Data == nullptr)
				pixels = jm::LoadImageFromFile(m_FileName, &texWidth, &texHeight, &texChannels);
			else
			{
				texWidth = m_Width;
				texHeight = m_Height;
				texChannels = 4;
				pixels = m_Data;
			}

			if(pixels == nullptr)
				return false;

			VkDeviceSize imageSize = texWidth * texHeight * 4;

			if (!pixels)
			{
				throw std::runtime_error("failed to load texture image!");
			}

			m_MipLevels = static_cast<uint32_t>(std::floor(std::log2(maths::Max(texWidth, texHeight)))) + 1;


			VkBuffer stagingBuffer;
			VkDeviceMemory stagingBufferMemory;
			VKTools::CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer,
				stagingBufferMemory);

			void* data;
			vkMapMemory(VKDevice::Instance()->GetDevice(), stagingBufferMemory, 0, imageSize, 0, &data);
			memcpy(data, pixels, static_cast<size_t>(imageSize));
			vkUnmapMemory(VKDevice::Instance()->GetDevice(), stagingBufferMemory);

			if (m_Data == nullptr)
				delete[] pixels;

			CreateImage(texWidth, texHeight, m_MipLevels, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_TextureImage, m_TextureImageMemory);


			VKTools::transitionImageLayout(m_TextureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_MipLevels);
			VKTools::copyBufferToImage(stagingBuffer, m_TextureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

			vkDestroyBuffer(VKDevice::Instance()->GetDevice(), stagingBuffer, nullptr);
			vkFreeMemory(VKDevice::Instance()->GetDevice(), stagingBufferMemory, nullptr);

			GenerateMipmaps(m_TextureImage, texWidth, texHeight, m_MipLevels);

			return true;
		}
	}
}
