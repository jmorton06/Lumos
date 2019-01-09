#include "JM.h"
#include "VKTextureCube.h"
#include "VKDevice.h"
#include "Utilities/LoadImage.h"
#include "VKTools.h"
#include "VKCommandBuffer.h"

#include "Maths/MathsUtilities.h"

#include <cmath>
#include "VKInitialisers.h"
#include "VKBuffer.h"

namespace jm
{
	namespace graphics
	{
		VKTextureCube::VKTextureCube(uint size)
		{
		}

		VKTextureCube::VKTextureCube(const String& name, const String* files)
		{
			m_Name = name;
			for (uint i = 0; i < 6; i++)
				m_Files[i] = files[i];

			Load(1);

			UpdateDescriptor();
		}

		VKTextureCube::VKTextureCube(const String& name, const String* files, uint mips, const InputFormat format)
		{
			m_Name = name;
			m_NumMips = mips;
			for (uint i = 0; i < mips; i++)
				m_Files[i] = files[i];

			Load(mips);

			UpdateDescriptor();
		}

		VKTextureCube::VKTextureCube(const String& name, const String& filepath) : m_Width(0), m_Height(0), m_Size(0), m_NumMips(0)
		{
			m_Name = name;
			m_Files[0] = filepath;
		}

		VKTextureCube::~VKTextureCube()
		{
			if (m_TextureSampler)
				vkDestroySampler(VKDevice::Instance()->GetDevice(), m_TextureSampler, nullptr);

			if (m_TextureImageView)
				vkDestroyImageView(VKDevice::Instance()->GetDevice(), m_TextureImageView, nullptr);

			if (m_DeleteImage)
			{
				vkDestroyImage(VKDevice::Instance()->GetDevice(), m_TextureImage, nullptr);
				vkFreeMemory(VKDevice::Instance()->GetDevice(), m_TextureImageMemory, nullptr);
			}
		}

		void VKTextureCube::Bind(uint slot) const
		{
		}

		void VKTextureCube::Unbind(uint slot) const
		{
		}

		VkFormat VKTextureCube::TextureFormatToVK(const TextureFormat format)
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
			default: JM_ERROR("[Texture] Unsupported image bit-depth!");  return VK_FORMAT_R8G8B8A8_UNORM;
			}
		}

		void VKTextureCube::CreateTextureSampler()
		{
			VkSamplerCreateInfo samplerInfo = {};
			samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerInfo.magFilter = VK_FILTER_LINEAR;
			samplerInfo.minFilter = VK_FILTER_LINEAR;
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
			samplerInfo.maxLod = static_cast<float>(m_NumMips);
			samplerInfo.mipLodBias = 0;

			if (vkCreateSampler(VKDevice::Instance()->GetDevice(), &samplerInfo, nullptr, &m_TextureSampler) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create texture sampler!");
			}
		}

		void VKTextureCube::CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling,
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
			// Cube faces count as array layers in Vulkan
			imageInfo.arrayLayers = 6;
			// This flag is required for cube map images
			imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

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

		VkImageView VKTextureCube::createImageView(VkImage image, VkFormat format, uint32_t mipLevels)
		{
			VkImageViewCreateInfo viewInfo = {};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = image;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
			viewInfo.format = format;
			viewInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
			viewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = mipLevels;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			// 6 array layers (faces)
			viewInfo.subresourceRange.layerCount = 6;

			VkImageView imageView;
			if (vkCreateImageView(VKDevice::Instance()->GetDevice(), &viewInfo, nullptr, &imageView) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create texture image view!");
			}

			return imageView;
		}

		void VKTextureCube::UpdateDescriptor()
		{
			m_Descriptor.sampler = m_TextureSampler;
			m_Descriptor.imageView = m_TextureImageView;
			m_Descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}

		void VKTextureCube::Load(uint mips)
		{
			uint srcWidth, srcHeight, bits;
			byte*** cubeTextureData = new byte**[mips];
			for (uint i = 0; i < mips; i++)
				cubeTextureData[i] = new byte*[6];

			uint* faceWidths = new uint[mips];
			uint* faceHeights = new uint[mips];
			uint size = 0;

			for (uint m = 0; m < mips; m++)
			{
				byte* data = jm::LoadImageFromFile(m_Files[m], &srcWidth, &srcHeight, &bits, !m_LoadOptions.flipY);
				//m_Parameters.format = VKTexture2D::BitsToTextureFormat(bits);
				uint stride = bits / 8;
				
				uint face = 0;
				uint faceWidth = srcWidth / 3;
				uint faceHeight = srcHeight / 4;
				faceWidths[m] = faceWidth;
				faceHeights[m] = faceHeight;
				for (uint cy = 0; cy < 4; cy++)
				{
					for (uint cx = 0; cx < 3; cx++)
					{
						if (cy == 0 || cy == 2 || cy == 3)
						{
							if (cx != 1)
								continue;
						}

						cubeTextureData[m][face] = new byte[faceWidth * faceHeight * stride];

						size += stride * srcHeight * srcWidth;

						for (uint y = 0; y < faceHeight; y++)
						{
							uint offset = y;
							if (face == 5)
								offset = faceHeight - (y + 1);
							uint yp = cy * faceHeight + offset;
							for (uint x = 0; x < faceWidth; x++)
							{
								offset = x;
								if (face == 5)
									offset = faceWidth - (x + 1);
								uint xp = cx * faceWidth + offset;
								cubeTextureData[m][face][(x + y * faceWidth) * stride + 0] = data[(xp + yp * srcWidth) * stride + 0];
								cubeTextureData[m][face][(x + y * faceWidth) * stride + 1] = data[(xp + yp * srcWidth) * stride + 1];
								cubeTextureData[m][face][(x + y * faceWidth) * stride + 2] = data[(xp + yp * srcWidth) * stride + 2];
								if (stride >= 4)
									cubeTextureData[m][face][(x + y * faceWidth) * stride + 3] = data[(xp + yp * srcWidth) * stride + 3];
							}
						}
						face++;
					}
				}
				delete[] data;
			}

			byte* allData = new byte[size];
			uint pointeroffset = 0;

			uint faceOrder[6] = { 3, 1, 0, 4, 2, 5 };

			for( uint face = 0; face < 6; face++)
			{
				for(uint mip = 0; mip < m_NumMips; mip++)
				{
					uint currentSize = faceWidths[mip] * faceHeights[mip] * bits / 8;
					memcpy(allData + pointeroffset, cubeTextureData[mip][faceOrder[face]], currentSize);
					pointeroffset += currentSize;
				}
			}
			VkBuffer stagingBuffer;
			VkDeviceMemory stagingBufferMemory;
			VKTools::CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer,
				stagingBufferMemory);

			void* data;
			vkMapMemory(VKDevice::Instance()->GetDevice(), stagingBufferMemory, 0, size, 0, &data);
			memcpy(data, allData, size);
			vkUnmapMemory(VKDevice::Instance()->GetDevice(), stagingBufferMemory);

            delete[] allData;
			CreateImage(faceWidths[0], faceHeights[0], m_NumMips, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_TextureImage, m_TextureImageMemory);

			VkCommandBuffer cmdBuffer;

			VkCommandBufferAllocateInfo cmdBufAllocateInfo =
				initializers::commandBufferAllocateInfo(
					VKContext::Get()->GetCommandPool()->GetCommandPool(),
					VK_COMMAND_BUFFER_LEVEL_PRIMARY,
					1);

			VK_CHECK_RESULT(vkAllocateCommandBuffers(VKDevice::Instance()->GetDevice(), &cmdBufAllocateInfo, &cmdBuffer));

			// If requested, also start the new command buffer
			VkCommandBufferBeginInfo cmdBufInfo = initializers::commandBufferBeginInfo();
			VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));

			// Setup buffer copy regions for each face including all of it's miplevels
			std::vector<VkBufferImageCopy> bufferCopyRegions;
			uint32_t offset = 0;

			for (uint32_t face = 0; face < 6; face++)
			{
				for (uint32_t level = 0; level < m_NumMips; level++)
				{
					VkBufferImageCopy bufferCopyRegion = {};
					bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					bufferCopyRegion.imageSubresource.mipLevel = level;
					bufferCopyRegion.imageSubresource.baseArrayLayer = face;
					bufferCopyRegion.imageSubresource.layerCount = 1;
					bufferCopyRegion.imageExtent.width = faceWidths[level];
					bufferCopyRegion.imageExtent.height = faceHeights[level];
					bufferCopyRegion.imageExtent.depth = 1;
					bufferCopyRegion.bufferOffset = offset;

					bufferCopyRegions.push_back(bufferCopyRegion);

					// Increase offset into staging buffer for next level / face
					offset += faceWidths[level] * faceWidths[level] * bits / 8;
				}
			}

			// Image barrier for optimal image (target)
			// Set initial layout for all array layers (faces) of the optimal (target) tiled texture
			VkImageSubresourceRange subresourceRange = {};
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = m_NumMips;
			subresourceRange.layerCount = 6;

			VKTools::setImageLayout(
				cmdBuffer,
				m_TextureImage,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				subresourceRange);

			// Copy the cube map faces from the staging buffer to the optimal tiled image
			vkCmdCopyBufferToImage(
				cmdBuffer,
				stagingBuffer,
				m_TextureImage,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				static_cast<uint32_t>(bufferCopyRegions.size()),
				bufferCopyRegions.data()
			);

			// Change texture image layout to shader read after all faces have been copied
			m_ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			VKTools::setImageLayout(
				cmdBuffer,
				m_TextureImage,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				m_ImageLayout,
				subresourceRange);

			if (cmdBuffer == VK_NULL_HANDLE)
			{
				return;
			}

			VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer));

			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &cmdBuffer;

			VK_CHECK_RESULT(vkQueueSubmit(VKDevice::Instance()->GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE));
			VK_CHECK_RESULT(vkQueueWaitIdle(VKDevice::Instance()->GetGraphicsQueue()));

			vkFreeCommandBuffers(VKDevice::Instance()->GetDevice(), VKContext::Get()->GetCommandPool()->GetCommandPool(), 1, &cmdBuffer);

			CreateTextureSampler();
			m_TextureImageView =  createImageView(m_TextureImage, VK_FORMAT_R8G8B8A8_UNORM, m_NumMips);

			for (uint m = 0; m < mips; m++)
			{
				for (uint f = 0; f < 6; f++)
				{
					delete[] cubeTextureData[m][f];
				}
				delete[] cubeTextureData[m];
			}
			delete[] cubeTextureData;
			delete[] faceHeights;
			delete[] faceWidths;
		}
	}
}
