#include "lmpch.h"
#include "VKTexture.h"
#include "VKDevice.h"
#include "Utilities/LoadImage.h"
#include "VKTools.h"
#include "VKBuffer.h"

namespace Lumos
{
	namespace Graphics
	{
		static VkImageView CreateImageView(VkImage image, VkFormat format, u32 mipLevels, VkImageViewType viewType, VkImageAspectFlags aspectMask, u32 layerCount, u32 baseArrayLayer = 0)
		{
			VkImageViewCreateInfo viewInfo = {};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = image;
			viewInfo.viewType = viewType;
			viewInfo.format = format;
			viewInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
			viewInfo.subresourceRange.aspectMask = aspectMask;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = mipLevels;
			viewInfo.subresourceRange.baseArrayLayer = baseArrayLayer;
			viewInfo.subresourceRange.layerCount = layerCount;

			VkImageView imageView;
			if (vkCreateImageView(VKDevice::Instance()->GetDevice(), &viewInfo, nullptr, &imageView) != VK_SUCCESS)
			{
				Lumos::Debug::Log::Error("Failed to create texture image view!");
			}

			return imageView;
		}

		static VkSampler CreateTextureSampler(VkFilter magFilter = VK_FILTER_LINEAR, VkFilter minFilter = VK_FILTER_LINEAR, float minLod = 0.0f, float maxLod = 1.0f, bool anisotropyEnable = false, float maxAnisotropy = 1.0f, VkSamplerAddressMode modeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VkSamplerAddressMode modeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VkSamplerAddressMode modeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
		{
			VkSampler sampler;
			VkSamplerCreateInfo samplerInfo = {};
			samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerInfo.magFilter = magFilter;
			samplerInfo.minFilter = minFilter;
			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			samplerInfo.addressModeU = modeU;
			samplerInfo.addressModeV = modeV;
			samplerInfo.addressModeW = modeW;
			samplerInfo.maxAnisotropy = maxAnisotropy;
			samplerInfo.anisotropyEnable = anisotropyEnable;
			samplerInfo.unnormalizedCoordinates = VK_FALSE;
			samplerInfo.compareEnable = VK_FALSE;
			samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
			samplerInfo.mipLodBias = 0.0f;
			samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
			samplerInfo.minLod = minLod;
			samplerInfo.maxLod = maxLod;

			if (vkCreateSampler(VKDevice::Instance()->GetDevice(), &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
			{
				Lumos::Debug::Log::Error("Failed to create texture sampler!");
			}

			return sampler;
		}

#ifdef USE_VMA_ALLOCATOR
		static void CreateImageVma(const VkImageCreateInfo& imageInfo, VkImage& image, VmaAllocation& allocation)
		{
			VmaAllocationCreateInfo allocInfovma;
			allocInfovma.flags = 0;
			allocInfovma.usage = VMA_MEMORY_USAGE_GPU_ONLY;
			allocInfovma.requiredFlags = 0;
			allocInfovma.preferredFlags = 0;
			allocInfovma.memoryTypeBits = 0;
			allocInfovma.pool = nullptr;
			allocInfovma.pUserData = nullptr;
			vmaCreateImage(VKDevice::Instance()->GetAllocator(), &imageInfo, &allocInfovma, &image, &allocation, nullptr);
		}

#else
		static void CreateImageDefault(const VkImageCreateInfo& imageInfo, VkImage& image,
			VkDeviceMemory& imageMemory, VkMemoryPropertyFlags properties)
		{
			vkCreateImage(VKDevice::Instance()->GetDevice(), &imageInfo, nullptr, &image);

			VkMemoryRequirements memRequirements;
			vkGetImageMemoryRequirements(VKDevice::Instance()->GetDevice(), image, &memRequirements);

			VkMemoryAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = memRequirements.size;
			allocInfo.memoryTypeIndex = VKTools::FindMemoryType(memRequirements.memoryTypeBits, properties);

			vkAllocateMemory(VKDevice::Instance()->GetDevice(), &allocInfo, nullptr, &imageMemory);
			vkBindImageMemory(VKDevice::Instance()->GetDevice(), image, imageMemory, 0);
		}
#endif

#ifdef USE_VMA_ALLOCATOR
		static void CreateImage(u32 width, u32 height, u32 mipLevels, VkFormat format, VkImageType imageType, VkImageTiling tiling,
			VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image,
			VkDeviceMemory& imageMemory, u32 arrayLayers, VkImageCreateFlags flags, VmaAllocation& allocation)
#else
        static void CreateImage(u32 width, u32 height, u32 mipLevels, VkFormat format, VkImageType imageType, VkImageTiling tiling,
            VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image,
            VkDeviceMemory& imageMemory, u32 arrayLayers, VkImageCreateFlags flags)
#endif
		{
			VkImageCreateInfo imageInfo = {};
			imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageInfo.imageType = imageType;
			imageInfo.extent = { width, height, 1 };
			imageInfo.mipLevels = mipLevels;
			imageInfo.format = format;
			imageInfo.tiling = tiling;
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageInfo.usage = usage;
			imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageInfo.arrayLayers = arrayLayers;

			imageInfo.flags = flags;

#ifdef USE_VMA_ALLOCATOR
			CreateImageVma(imageInfo, image, allocation);
#else
			CreateImageDefault(imageInfo, image, imageMemory, properties);
#endif
		}

		VKTexture2D::VKTexture2D(u32 width, u32 height, void* data, TextureParameters parameters, TextureLoadOptions loadOptions)
			: m_FileName("NULL"), m_TextureImage(VK_NULL_HANDLE), m_TextureImageView(VK_NULL_HANDLE), m_TextureSampler(VK_NULL_HANDLE)
		{
			m_Width = width;
			m_Height = height;
			m_Parameters = parameters;
			m_LoadOptions = loadOptions;
			m_Data = static_cast<u8*>(data);
			Load();

			m_TextureImageView = Graphics::CreateImageView(m_TextureImage, VK_FORMAT_R8G8B8A8_UNORM, m_MipLevels, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, 1);
			m_TextureSampler = Graphics::CreateTextureSampler(m_Parameters.filter == TextureFilter::LINEAR ? VK_FILTER_LINEAR : VK_FILTER_NEAREST, m_Parameters.filter == TextureFilter::LINEAR ? VK_FILTER_LINEAR : VK_FILTER_NEAREST, 0.0f, static_cast<float>(m_MipLevels), true, VKDevice::Instance()->GetGPUProperties().limits.maxSamplerAnisotropy, VKTools::TextureWrapToVK(m_Parameters.wrap), VKTools::TextureWrapToVK(m_Parameters.wrap), VKTools::TextureWrapToVK(m_Parameters.wrap));

			UpdateDescriptor();
		}

		VKTexture2D::VKTexture2D(const String& name, const String& filename, TextureParameters parameters,
			TextureLoadOptions loadOptions)
			: m_FileName(filename), m_TextureImage(VK_NULL_HANDLE), m_TextureImageView(VK_NULL_HANDLE), m_TextureSampler(VK_NULL_HANDLE)
		{
			m_Parameters = parameters;
			m_LoadOptions = loadOptions;
			m_DeleteImage = Load();

			if (!m_DeleteImage)
				return;

			m_TextureImageView = Graphics::CreateImageView(m_TextureImage, VK_FORMAT_R8G8B8A8_UNORM, m_MipLevels, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, 1);
			m_TextureSampler = Graphics::CreateTextureSampler(m_Parameters.filter == TextureFilter::LINEAR ? VK_FILTER_LINEAR : VK_FILTER_NEAREST, m_Parameters.filter == TextureFilter::LINEAR ? VK_FILTER_LINEAR : VK_FILTER_NEAREST, 0.0f, static_cast<float>(m_MipLevels), true, VKDevice::Instance()->GetGPUProperties().limits.maxSamplerAnisotropy, VKTools::TextureWrapToVK(m_Parameters.wrap), VKTools::TextureWrapToVK(m_Parameters.wrap), VKTools::TextureWrapToVK(m_Parameters.wrap));

			UpdateDescriptor();
		}

        VKTexture2D::VKTexture2D(VkImage image, VkImageView imageView) : m_TextureImage(image), m_TextureImageView(imageView), m_TextureSampler(VK_NULL_HANDLE)
		{
			m_DeleteImage = false;
			m_TextureImageMemory = VK_NULL_HANDLE;

			UpdateDescriptor();
		}

		VKTexture2D::VKTexture2D()
			: m_FileName("NULL"), m_TextureSampler(VK_NULL_HANDLE), m_TextureImageView(VK_NULL_HANDLE)
		{
			m_Width = 0;
			m_Height = 0;
			m_Parameters = TextureParameters();
			m_LoadOptions = TextureLoadOptions();
			m_DeleteImage = false;
			m_TextureSampler = Graphics::CreateTextureSampler(m_Parameters.filter == TextureFilter::LINEAR ? VK_FILTER_LINEAR : VK_FILTER_NEAREST, m_Parameters.filter == TextureFilter::LINEAR ? VK_FILTER_LINEAR : VK_FILTER_NEAREST, 0.0f, static_cast<float>(m_MipLevels), true, VKDevice::Instance()->GetGPUProperties().limits.maxSamplerAnisotropy, VKTools::TextureWrapToVK(m_Parameters.wrap), VKTools::TextureWrapToVK(m_Parameters.wrap), VKTools::TextureWrapToVK(m_Parameters.wrap));

			UpdateDescriptor();
		}

		VKTexture2D::~VKTexture2D()
		{
			if (m_TextureSampler)
				vkDestroySampler(VKDevice::Device(), m_TextureSampler, nullptr);

			if (m_TextureImageView)
				vkDestroyImageView(VKDevice::Device(), m_TextureImageView, nullptr);

			if (m_DeleteImage)
			{
#ifdef USE_VMA_ALLOCATOR
                vmaDestroyImage(VKDevice::Instance()->GetAllocator(), m_TextureImage, m_Allocation);
#else
               vkDestroyImage(VKDevice::Instance()->GetDevice(), m_TextureImage, nullptr);

				if (m_TextureImageMemory)
				{
					vkFreeMemory(VKDevice::Instance()->GetDevice(), m_TextureImageMemory, nullptr);
				}
#endif
			}
		}

		void VKTexture2D::BuildTexture(TextureFormat internalformat, u32 width, u32 height, bool depth, bool samplerShadow)
		{
			m_Width = width;
			m_Height = height;
			m_Handle = 0;
			m_DeleteImage = true;
			m_MipLevels = 1;
            
#ifdef USE_VMA_ALLOCATOR
			Graphics::CreateImage(m_Width, m_Height, m_MipLevels, VKTools::TextureFormatToVK(internalformat), VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_TextureImage,
				m_TextureImageMemory, 1, 0, m_Allocation);
#else
            Graphics::CreateImage(m_Width, m_Height, m_MipLevels, VKTools::TextureFormatToVK(internalformat), VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_TextureImage,
            m_TextureImageMemory, 1, 0);
#endif
			
			m_TextureImageView = Graphics::CreateImageView(m_TextureImage, VKTools::TextureFormatToVK(internalformat), m_MipLevels, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, 1);

			UpdateDescriptor();
		}

		void VKTexture2D::UpdateDescriptor()
		{
			m_Descriptor.sampler = m_TextureSampler;
			m_Descriptor.imageView = m_TextureImageView;
            m_Descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}

		void GenerateMipmaps(VkImage image, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) 
		{
            VkCommandBuffer commandBuffer = VKTools::BeginSingleTimeCommands();

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

			for (uint32_t i = 1; i < mipLevels; i++)
            {
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
                blit.srcOffsets[0] = VkOffset3D{ 0, 0, 0 };
                blit.srcOffsets[1] = VkOffset3D{ mipWidth, mipHeight, 1 };
                blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				blit.srcSubresource.mipLevel = i - 1;
				blit.srcSubresource.baseArrayLayer = 0;
				blit.srcSubresource.layerCount = 1;
				blit.dstOffsets[0] = VkOffset3D{ 0, 0, 0 };
				blit.dstOffsets[1] = VkOffset3D{ mipWidth / 2, mipHeight / 2, 1 };
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

			VKTools::EndSingleTimeCommands(commandBuffer);
		}


		bool VKTexture2D::Load()
		{
			u32 texWidth, texHeight, texChannels;
			u8* pixels;

			if (m_Data == nullptr)
				pixels = Lumos::LoadImageFromFile(m_FileName, &texWidth, &texHeight, &texChannels);
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
				Debug::Log::Critical("failed to load texture image!");
			}

			m_MipLevels = static_cast<uint32_t>(std::floor(std::log2(Maths::Max(texWidth, texHeight)))) + 1;

			VKBuffer* stagingBuffer = lmnew VKBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, static_cast<u32>(imageSize), pixels);

			if (m_Data == nullptr)
				delete[] pixels;
#ifdef USE_VMA_ALLOCATOR
			Graphics::CreateImage(texWidth, texHeight, m_MipLevels, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_TextureImage,
				m_TextureImageMemory, 1, 0, m_Allocation);
#else
            Graphics::CreateImage(texWidth, texHeight, m_MipLevels, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_TextureImage, m_TextureImageMemory, 1, 0);
#endif

			VKTools::TransitionImageLayout(m_TextureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_MipLevels);
			VKTools::CopyBufferToImage(stagingBuffer->GetBuffer(), m_TextureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
			VKTools::TransitionImageLayout(m_TextureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_MipLevels);

			delete stagingBuffer;

			GenerateMipmaps(m_TextureImage, texWidth, texHeight, m_MipLevels);

			return true;
		}

		VKTextureCube::VKTextureCube(u32 size): m_ImageLayout()
		{
		}

		VKTextureCube::VKTextureCube(const String* files)
		{
			for (u32 i = 0; i < 6; i++)
				m_Files[i] = files[i];

			Load(1);

			UpdateDescriptor();
		}

		VKTextureCube::VKTextureCube(const String* files, u32 mips, const InputFormat format)
		{
			m_NumMips = mips;
			for (u32 i = 0; i < mips; i++)
				m_Files[i] = files[i];

			Load(mips);

			UpdateDescriptor();
		}

		VKTextureCube::VKTextureCube(const String& filepath) : 
			m_Width(0), 
			m_Height(0),
			m_Size(0), 
			m_NumMips(0),
			m_ImageLayout()
		{
			m_Files[0] = filepath;
		}

		VKTextureCube::~VKTextureCube()
		{
			if (m_TextureSampler)
				vkDestroySampler(VKDevice::Device(), m_TextureSampler, nullptr);

			if (m_TextureImageView)
				vkDestroyImageView(VKDevice::Device(), m_TextureImageView, nullptr);

			if (m_DeleteImage)
			{
#ifdef USE_VMA_ALLOCATOR
				vmaDestroyImage(VKDevice::Instance()->GetAllocator(), m_TextureImage, m_Allocation);
#else
				vkDestroyImage(VKDevice::Instance()->GetDevice(), m_TextureImage, nullptr);
				vkFreeMemory(VKDevice::Instance()->GetDevice(), m_TextureImageMemory, nullptr);
#endif

			}
		}

		void VKTextureCube::UpdateDescriptor()
		{
			m_Descriptor.sampler = m_TextureSampler;
			m_Descriptor.imageView = m_TextureImageView;
			m_Descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}

		void VKTextureCube::Load(u32 mips)
		{
			u32 srcWidth, srcHeight, bits;
			u8*** cubeTextureData = lmnew u8**[mips];
			for (u32 i = 0; i < mips; i++)
				cubeTextureData[i] = lmnew u8*[6];

			u32* faceWidths = lmnew u32[mips];
			u32* faceHeights = lmnew u32[mips];
			u32 size = 0;

			for (u32 m = 0; m < mips; m++)
			{
				u8* data = Lumos::LoadImageFromFile(m_Files[m], &srcWidth, &srcHeight, &bits, !m_LoadOptions.flipY);
				//m_Parameters.format = VKTexture2D::BitsToTextureFormat(bits);
				u32 stride = bits / 8;

				u32 face = 0;
				u32 faceWidth = srcWidth / 3;
				u32 faceHeight = srcHeight / 4;
				faceWidths[m] = faceWidth;
				faceHeights[m] = faceHeight;
				for (u32 cy = 0; cy < 4; cy++)
				{
					for (u32 cx = 0; cx < 3; cx++)
					{
						if (cy == 0 || cy == 2 || cy == 3)
						{
							if (cx != 1)
								continue;
						}

						cubeTextureData[m][face] = lmnew u8[faceWidth * faceHeight * stride];

						size += stride * srcHeight * srcWidth;

						for (u32 y = 0; y < faceHeight; y++)
						{
							u32 offset = y;
							if (face == 5)
								offset = faceHeight - (y + 1);
							u32 yp = cy * faceHeight + offset;
							for (u32 x = 0; x < faceWidth; x++)
							{
								offset = x;
								if (face == 5)
									offset = faceWidth - (x + 1);
								u32 xp = cx * faceWidth + offset;
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

			u8* allData = lmnew u8[size];
			u32 pointeroffset = 0;

			u32 faceOrder[6] = { 3, 1, 0, 4, 2, 5 };

			for (u32 face = 0; face < 6; face++)
			{
				for (u32 mip = 0; mip < m_NumMips; mip++)
				{
					u32 currentSize = faceWidths[mip] * faceHeights[mip] * bits / 8;
					memcpy(allData + pointeroffset, cubeTextureData[mip][faceOrder[face]], currentSize);
					pointeroffset += currentSize;
				}
			}

			VKBuffer* stagingBuffer = lmnew VKBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, static_cast<u32>(size), allData);

			if (m_Data == nullptr)
			{
				lmdel[] allData;
				allData = nullptr;
			}
#ifdef USE_VMA_ALLOCATOR
			Graphics::CreateImage(faceWidths[0], faceHeights[0], m_NumMips, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_TextureImage,
				m_TextureImageMemory, 6, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, m_Allocation);
#else
            Graphics::CreateImage(faceWidths[0], faceHeights[0], m_NumMips, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_TextureImage, m_TextureImageMemory, 6, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT);
#endif

			VkCommandBuffer cmdBuffer = VKTools::BeginSingleTimeCommands();

			//// Setup buffer copy regions for each face including all of it's miplevels
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

			VKTools::SetImageLayout(
				cmdBuffer,
				m_TextureImage,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				subresourceRange);

			// Copy the cube map faces from the staging buffer to the optimal tiled image
			vkCmdCopyBufferToImage(
				cmdBuffer,
				stagingBuffer->GetBuffer(),
				m_TextureImage,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				static_cast<uint32_t>(bufferCopyRegions.size()),
				bufferCopyRegions.data()
			);

			// Change texture image layout to shader read after all faces have been copied
			m_ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VKTools::SetImageLayout(
				cmdBuffer,
				m_TextureImage,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				m_ImageLayout,
				subresourceRange);

			VKTools::EndSingleTimeCommands(cmdBuffer);

			m_TextureSampler = Graphics::CreateTextureSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, 0.0f, static_cast<float>(m_NumMips), true, VKDevice::Instance()->GetGPUProperties().limits.maxSamplerAnisotropy, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT);
			m_TextureImageView = Graphics::CreateImageView(m_TextureImage, VK_FORMAT_R8G8B8A8_UNORM, m_NumMips, VK_IMAGE_VIEW_TYPE_CUBE, VK_IMAGE_ASPECT_COLOR_BIT, 6);

			delete stagingBuffer;

			for (u32 m = 0; m < mips; m++)
			{
				for (u32 f = 0; f < 6; f++)
				{
					delete[] cubeTextureData[m][f];
				}
				delete[] cubeTextureData[m];
			}
			delete[] cubeTextureData;
			delete[] faceHeights;
			delete[] faceWidths;
			delete[] allData;
		}

		VKTextureDepth::VKTextureDepth(u32 width, u32 height)
			: m_Width(width), m_Height(height), m_TextureImageView(VK_NULL_HANDLE), m_TextureSampler(VK_NULL_HANDLE)
		{
			Init();
		}

		VKTextureDepth::~VKTextureDepth()
		{
			if (m_TextureSampler)
				vkDestroySampler(VKDevice::Device(), m_TextureSampler, nullptr);

			vkDestroyImageView(VKDevice::Device(), m_TextureImageView, nullptr);
#ifdef USE_VMA_ALLOCATOR
			vmaDestroyImage(VKDevice::Instance()->GetAllocator(), m_TextureImage, m_Allocation);
#else
			vkDestroyImage(VKDevice::Instance()->GetDevice(), m_TextureImage, nullptr);
			vkFreeMemory(VKDevice::Instance()->GetDevice(), m_TextureImageMemory, nullptr);
#endif
		}

		void VKTextureDepth::Init()
		{
			VkFormat depthFormat = VKTools::FindDepthFormat();
#ifdef USE_VMA_ALLOCATOR
			Graphics::CreateImage(m_Width, m_Height, 1, depthFormat, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_TextureImage,
				m_TextureImageMemory, 1, 0, m_Allocation);
#else
            Graphics::CreateImage(m_Width, m_Height, 1, depthFormat, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_TextureImage, m_TextureImageMemory, 1, 0);
#endif
			m_TextureImageView = CreateImageView(m_TextureImage, depthFormat, 1, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

			VKTools::TransitionImageLayout(m_TextureImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

			m_TextureSampler = Graphics::CreateTextureSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, 0.0f, 1.0f, true, VKDevice::Instance()->GetGPUProperties().limits.maxSamplerAnisotropy, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT);

			UpdateDescriptor();
		}

		void VKTextureDepth::UpdateDescriptor()
		{
			m_Descriptor.sampler = m_TextureSampler;
			m_Descriptor.imageView = m_TextureImageView;
			m_Descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}

		void VKTextureDepth::Resize(u32 width, u32 height)
		{
			m_Width = width;
			m_Height = height;

			if (m_TextureSampler)
				vkDestroySampler(VKDevice::Device(), m_TextureSampler, nullptr);

			vkDestroyImageView(VKDevice::Device(), m_TextureImageView, nullptr);
#ifdef USE_VMA_ALLOCATOR
			vmaDestroyImage(VKDevice::Instance()->GetAllocator(), m_TextureImage, m_Allocation);
#else
			vkDestroyImage(VKDevice::Instance()->GetDevice(), m_TextureImage, nullptr);
			vkFreeMemory(VKDevice::Instance()->GetDevice(), m_TextureImageMemory, nullptr);
#endif

			Init();
		}

		VKTextureDepthArray::VKTextureDepthArray(u32 width, u32 height, u32 count)
			: m_Width(width), m_Height(height), m_Count(count)
		{
			VKTextureDepthArray::Init();
		}

		VKTextureDepthArray::~VKTextureDepthArray()
		{
            vkDestroyImageView(VKDevice::Device(), m_TextureImageView, nullptr);

#ifdef USE_VMA_ALLOCATOR
			vmaDestroyImage(VKDevice::Instance()->GetAllocator(), m_TextureImage, m_Allocation);
#else
			vkDestroyImage(VKDevice::Instance()->GetDevice(), m_TextureImage, nullptr);
			vkFreeMemory(VKDevice::Instance()->GetDevice(), m_TextureImageMemory, nullptr);
#endif
            vkDestroySampler(VKDevice::Device(), m_TextureSampler, nullptr);

			for (uint32_t i = 0; i < m_Count; i++)
			{
                vkDestroyImageView(VKDevice::Device(), m_IndividualImageViews[i], nullptr);
			}
		}

		void VKTextureDepthArray::Init()
		{
			VkFormat depthFormat = VKTools::FindDepthFormat();
#ifdef USE_VMA_ALLOCATOR
			Graphics::CreateImage(m_Width, m_Height, 1, depthFormat, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_TextureImage,
				m_TextureImageMemory, m_Count, 0, m_Allocation);
#else
            Graphics::CreateImage(m_Width, m_Height, 1, depthFormat, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL,
                        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_TextureImage,
                        m_TextureImageMemory, m_Count, 0);
#endif
			m_TextureImageView = CreateImageView(m_TextureImage, depthFormat, 1, VK_IMAGE_VIEW_TYPE_2D_ARRAY, VK_IMAGE_ASPECT_DEPTH_BIT, m_Count);

			for (uint32_t i = 0; i < m_Count; i++)
			{
				VkImageView imageView = CreateImageView(m_TextureImage, depthFormat, 1, VK_IMAGE_VIEW_TYPE_2D_ARRAY, VK_IMAGE_ASPECT_DEPTH_BIT, 1, i);
				m_IndividualImageViews.push_back(imageView);
			}

			VKTools::TransitionImageLayout(m_TextureImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

			m_TextureSampler = Graphics::CreateTextureSampler();

			UpdateDescriptor();
		}

		void VKTextureDepthArray::UpdateDescriptor()
		{
			m_Descriptor.sampler = m_TextureSampler;
			m_Descriptor.imageView = m_TextureImageView;
			m_Descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}

		void* VKTextureDepthArray::GetHandleArray(u32 index) 
		{
			m_Descriptor.imageView = GetImageView(index); 
			return (void*)&m_Descriptor;
		}

		void VKTextureDepthArray::Resize(u32 width, u32 height, u32 count)
		{
			m_Width = width;
			m_Height = height;
			m_Count = count;


			if (m_TextureSampler)
				vkDestroySampler(VKDevice::Device(), m_TextureSampler, nullptr);

			vkDestroyImageView(VKDevice::Device(), m_TextureImageView, nullptr);
#ifdef USE_VMA_ALLOCATOR
			vmaDestroyImage(VKDevice::Instance()->GetAllocator(), m_TextureImage, m_Allocation);
#else
			vkDestroyImage(VKDevice::Instance()->GetDevice(), m_TextureImage, nullptr);
			vkFreeMemory(VKDevice::Instance()->GetDevice(), m_TextureImageMemory, nullptr);
#endif

			Init();
		}

		Texture2D* VKTexture2D::CreateFuncVulkan()
		{
			return lmnew VKTexture2D();
		}

		Texture2D* VKTexture2D::CreateFromSourceFuncVulkan(u32 width, u32 height, void* data, TextureParameters parameters, TextureLoadOptions loadoptions)
		{
			return lmnew VKTexture2D(width, height, data, parameters, loadoptions);
		}

		Texture2D* VKTexture2D::CreateFromFileFuncVulkan(const String& name, const String& filename, TextureParameters parameters, TextureLoadOptions loadoptions)
		{
			return lmnew VKTexture2D(name, filename, parameters, loadoptions);
		}
        
		TextureCube* VKTextureCube::CreateFuncVulkan(u32 size)
		{
			return lmnew VKTextureCube(size);
		}

		TextureCube* VKTextureCube::CreateFromFileFuncVulkan(const String& filepath)
		{
			return lmnew VKTextureCube(filepath);
		}

		TextureCube* VKTextureCube::CreateFromFilesFuncVulkan(const String* files)
		{
			return lmnew VKTextureCube(files);
		}

		TextureCube* VKTextureCube::CreateFromVCrossFuncVulkan(const String* files, u32 mips, InputFormat format)
		{
			return lmnew VKTextureCube(files, mips, format);
		}

		TextureDepth* VKTextureDepth::CreateFuncVulkan(u32 width, u32 height)
		{
			return lmnew VKTextureDepth(width, height);
		}

		TextureDepthArray* VKTextureDepthArray::CreateFuncVulkan(u32 width, u32 height, u32 count)
		{
			return lmnew VKTextureDepthArray(width, height, count);
		}

		void VKTexture2D::MakeDefault()
		{
			CreateFunc = CreateFuncVulkan;
			CreateFromFileFunc = CreateFromFileFuncVulkan;
			CreateFromSourceFunc = CreateFromSourceFuncVulkan;
		}

        void VKTextureCube::MakeDefault()
        {
            CreateFunc = CreateFuncVulkan;
			CreateFromFileFunc = CreateFromFileFuncVulkan;
			CreateFromFilesFunc = CreateFromFilesFuncVulkan;
			CreateFromVCrossFunc = CreateFromVCrossFuncVulkan;
        }

		void VKTextureDepth::MakeDefault()
		{
			CreateFunc = CreateFuncVulkan;
		}

		void VKTextureDepthArray::MakeDefault()
		{
			CreateFunc = CreateFuncVulkan;
		}
	}
}
