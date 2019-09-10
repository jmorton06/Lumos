#include "LM.h"
#include "VKTexture.h"
#include "VKDevice.h"
#include "Utilities/LoadImage.h"
#include "VKTools.h"
#include "VKCommandBuffer.h"
#include "VKBuffer.h"

#include "Maths/MathsUtilities.h"

#include <cmath>

namespace Lumos
{
	namespace Graphics
	{
		VKTexture2D::VKTexture2D(u32 width, u32 height, void* data, TextureParameters parameters, TextureLoadOptions loadOptions)
			: m_FileName("NULL"), m_TextureSampler(nullptr), m_TextureImageView(nullptr)
		{
			m_Width = width;
			m_Height = height;
			m_Parameters = parameters;
			m_LoadOptions = loadOptions;
			m_Data = static_cast<u8*>(data);
			Load();

			m_TextureImageView = CreateImageView(m_TextureImage, vk::Format::eR8G8B8A8Unorm,  vk::ImageAspectFlagBits::eColor, m_MipLevels);

			CreateTextureSampler();

			UpdateDescriptor();
		}

		VKTexture2D::VKTexture2D(const String& name, const String& filename, TextureParameters parameters,
			TextureLoadOptions loadOptions)
			: m_FileName(filename), m_TextureSampler(nullptr), m_TextureImageView(nullptr)
		{
			m_Parameters = parameters;
			m_LoadOptions = loadOptions;
			m_DeleteImage = Load();

			if(!m_DeleteImage)
				return;

            m_TextureImageView = CreateImageView(m_TextureImage, vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor, m_MipLevels);

			CreateTextureSampler();
			UpdateDescriptor();
		}

        VKTexture2D::VKTexture2D(vk::Image image, vk::ImageView imageView) : m_TextureImage(image), m_TextureImageView(imageView), m_TextureSampler(nullptr)
		{
			m_DeleteImage = false;
			m_TextureImageMemory = nullptr;

			UpdateDescriptor();
		}

		VKTexture2D::VKTexture2D()
			: m_FileName("NULL"), m_TextureSampler(nullptr), m_TextureImageView(nullptr)
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
			if (m_TextureSampler)
				VKDevice::Instance()->GetDevice().destroySampler(m_TextureSampler);

			if (m_TextureImageView)
				VKDevice::Instance()->GetDevice().destroyImageView(m_TextureImageView);

			if (m_DeleteImage)
			{
#ifdef USE_VMA_ALLOCATOR
                vmaDestroyImage(VKDevice::Instance()->GetAllocator(), m_TextureImage, m_Allocation);
#else
                VKDevice::Instance()->GetDevice().destroyImage(m_TextureImage);

				if (m_TextureImageMemory)
				{
					VKDevice::Instance()->GetDevice().freeMemory(m_TextureImageMemory);
				}
#endif
			}
		}

		void VKTexture2D::Bind(u32 slot) const
		{
		}

		void VKTexture2D::Unbind(u32 slot) const
		{
		}

        vk::SamplerAddressMode TextureWrapToVK(const TextureWrap wrap)
		{
			switch (wrap)
			{
            case TextureWrap::CLAMP:		    return vk::SamplerAddressMode::eClampToEdge;
			case TextureWrap::CLAMP_TO_BORDER:	return vk::SamplerAddressMode::eClampToBorder;
			case TextureWrap::CLAMP_TO_EDGE:	return vk::SamplerAddressMode::eClampToEdge;
			case TextureWrap::REPEAT:			return vk::SamplerAddressMode::eRepeat;
			case TextureWrap::MIRRORED_REPEAT:	return vk::SamplerAddressMode::eMirroredRepeat;
			default: LUMOS_LOG_CRITICAL("[Texture] Unsupported wrap type!");  return vk::SamplerAddressMode::eClampToEdge;
			}
		}

		void VKTexture2D::BuildTexture(TextureFormat internalformat, u32 width, u32 height, bool depth, bool samplerShadow)
		{
			m_Width = width;
			m_Height = height;
			m_Handle = 0;
			m_DeleteImage = true;
			m_MipLevels = 1;
			CreateImage(m_Width, m_Height, m_MipLevels, VKTools::TextureFormatToVK(internalformat), vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal, m_TextureImage,
				m_TextureImageMemory);

			m_TextureImageView = CreateImageView(m_TextureImage, VKTools::TextureFormatToVK(internalformat), vk::ImageAspectFlagBits::eColor, m_MipLevels);

			UpdateDescriptor();
		}

		void VKTexture2D::CreateTextureSampler()
		{
            vk::SamplerCreateInfo samplerInfo = {};
            samplerInfo.magFilter = m_Parameters.filter == TextureFilter::LINEAR ? vk::Filter::eLinear : vk::Filter::eNearest;
			samplerInfo.minFilter = m_Parameters.filter == TextureFilter::LINEAR ? vk::Filter::eLinear : vk::Filter::eNearest;
            samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
			samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
			samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
			samplerInfo.anisotropyEnable = VK_TRUE;
			samplerInfo.maxAnisotropy = VKDevice::Instance()->GetGPUProperties().limits.maxSamplerAnisotropy;
            samplerInfo.borderColor = vk::BorderColor::eFloatOpaqueBlack;
			samplerInfo.unnormalizedCoordinates = VK_FALSE;
			samplerInfo.compareEnable = VK_FALSE;
            samplerInfo.compareOp = vk::CompareOp::eNever;
            samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
			samplerInfo.minLod = 0;
			samplerInfo.maxLod = static_cast<float>(m_MipLevels);
			samplerInfo.mipLodBias = 0;
			samplerInfo.addressModeU = TextureWrapToVK(m_Parameters.wrap);
			samplerInfo.addressModeV = TextureWrapToVK(m_Parameters.wrap);
			samplerInfo.addressModeW = TextureWrapToVK(m_Parameters.wrap);

			m_TextureSampler = VKDevice::Instance()->GetDevice().createSampler(samplerInfo);
		}

        void VKTexture2D::CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, vk::Format format, vk::ImageTiling tiling,
                                      vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Image& image,
                                      vk::DeviceMemory& imageMemory)
		{
            vk::ImageCreateInfo imageInfo = {};
            imageInfo.imageType = vk::ImageType::e2D;
			imageInfo.extent.width = width;
			imageInfo.extent.height = height;
			imageInfo.extent.depth = 1;
			imageInfo.mipLevels = mipLevels;
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

        vk::ImageView VKTexture2D::CreateImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels)
		{
            vk::ImageViewCreateInfo viewInfo = {};
			viewInfo.image = image;
            viewInfo.viewType = vk::ImageViewType::e2D;
			viewInfo.format = format;
			viewInfo.subresourceRange.aspectMask = aspectFlags;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = mipLevels;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			vk::ImageView imageView = VKDevice::Instance()->GetDevice().createImageView(viewInfo);
			return imageView;
		}

		void VKTexture2D::UpdateDescriptor()
		{
			m_Descriptor.sampler = m_TextureSampler;
			m_Descriptor.imageView = m_TextureImageView;
            m_Descriptor.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		}

		void GenerateMipmaps(vk::Image image, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) 
		{
            vk::CommandBuffer commandBuffer = VKTools::BeginSingleTimeCommands();

            vk::ImageMemoryBarrier barrier = {};
			barrier.image = image;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;
			barrier.subresourceRange.levelCount = 1;

			int32_t mipWidth = texWidth;
			int32_t mipHeight = texHeight;

			for (uint32_t i = 1; i < mipLevels; i++)
            {
				barrier.subresourceRange.baseMipLevel = i - 1;
                barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
                barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
                barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
                barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

                commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, static_cast<vk::DependencyFlagBits>(0), 0, nullptr, 0, nullptr, 1, &barrier);

                vk::ImageBlit blit = {};
                blit.srcOffsets[0] = vk::Offset3D{ 0, 0, 0 };
                blit.srcOffsets[1] = vk::Offset3D{ mipWidth, mipHeight, 1 };
                blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
				blit.srcSubresource.mipLevel = i - 1;
				blit.srcSubresource.baseArrayLayer = 0;
				blit.srcSubresource.layerCount = 1;
				blit.dstOffsets[0] = vk::Offset3D{ 0, 0, 0 };
				blit.dstOffsets[1] = vk::Offset3D{ mipWidth / 2, mipHeight / 2, 1 };
                blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
				blit.dstSubresource.mipLevel = i;
				blit.dstSubresource.baseArrayLayer = 0;
				blit.dstSubresource.layerCount = 1;

                commandBuffer.blitImage(image, vk::ImageLayout::eTransferSrcOptimal, image, vk::ImageLayout::eTransferDstOptimal, 1, &blit, vk::Filter::eLinear);

                barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
                barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
                barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
                barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

                
				commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, static_cast<vk::DependencyFlagBits>(0), 0, nullptr, 0, nullptr, 1, &barrier);

				if (mipWidth > 1) mipWidth /= 2;
				if (mipHeight > 1) mipHeight /= 2;
			}

			barrier.subresourceRange.baseMipLevel = mipLevels - 1;
            barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
            barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

			commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, static_cast<vk::DependencyFlagBits>(0), 0, nullptr, 0, nullptr, 1, &barrier);

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

			vk::DeviceSize imageSize = texWidth * texHeight * 4;

			if (!pixels)
			{
                LUMOS_LOG_CRITICAL("failed to load texture image!");
			}

			m_MipLevels = static_cast<uint32_t>(std::floor(std::log2(Maths::Max(texWidth, texHeight)))) + 1;

			VKBuffer* stagingBuffer = lmnew VKBuffer(vk::BufferUsageFlagBits::eTransferSrc, static_cast<u32>(imageSize), pixels);

			if (m_Data == nullptr)
				delete[] pixels;

            CreateImage(texWidth, texHeight, m_MipLevels, vk::Format::eR8G8B8A8Unorm, vk::ImageTiling::eOptimal,  vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal, m_TextureImage, m_TextureImageMemory);

			VKTools::TransitionImageLayout(m_TextureImage, vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, m_MipLevels);
			VKTools::CopyBufferToImage(stagingBuffer->GetBuffer(), m_TextureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

			delete stagingBuffer;

			GenerateMipmaps(m_TextureImage, texWidth, texHeight, m_MipLevels);

			return true;
		}

		VKTextureCube::VKTextureCube(u32 size)
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

		VKTextureCube::VKTextureCube(const String& filepath) : m_Width(0), m_Height(0), m_Size(0), m_NumMips(0)
		{
			m_Files[0] = filepath;
		}

		VKTextureCube::~VKTextureCube()
		{
			if (m_TextureSampler)
				VKDevice::Instance()->GetDevice().destroySampler(m_TextureSampler);

			if (m_TextureImageView)
				VKDevice::Instance()->GetDevice().destroyImageView(m_TextureImageView);

			if (m_DeleteImage)
			{
#ifdef USE_VMA_ALLOCATOR
				vmaDestroyImage(VKDevice::Instance()->GetAllocator(), m_TextureImage, m_Allocation);
#else
				VKDevice::Instance()->GetDevice().destroyImage(m_TextureImage);
				VKDevice::Instance()->GetDevice().freeMemory(m_TextureImageMemory);
#endif

			}
		}

		void VKTextureCube::Bind(u32 slot) const
		{
		}

		void VKTextureCube::Unbind(u32 slot) const
		{
		}

		void VKTextureCube::CreateTextureSampler()
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
			samplerInfo.compareOp = vk::CompareOp::eNever;
			samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
			samplerInfo.minLod = 0;
			samplerInfo.maxLod = static_cast<float>(m_NumMips);
			samplerInfo.mipLodBias = 0;

			m_TextureSampler = VKDevice::Instance()->GetDevice().createSampler(samplerInfo);
		}

		void VKTextureCube::CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, vk::Format format, vk::ImageTiling tiling,
			vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Image& image,
			vk::DeviceMemory& imageMemory)
		{
			vk::ImageCreateInfo imageInfo = {};
			imageInfo.imageType = vk::ImageType::e2D;
			imageInfo.extent.width = width;
			imageInfo.extent.height = height;
			imageInfo.extent.depth = 1;
			imageInfo.mipLevels = mipLevels;
			imageInfo.arrayLayers = 6;
			imageInfo.format = format;
			imageInfo.tiling = tiling;
			imageInfo.initialLayout = vk::ImageLayout::eUndefined;
			imageInfo.usage = usage;
			imageInfo.samples = vk::SampleCountFlagBits::e1;
			imageInfo.sharingMode = vk::SharingMode::eExclusive;
			imageInfo.flags = vk::ImageCreateFlagBits::eCubeCompatible;

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

		vk::ImageView VKTextureCube::CreateImageView(vk::Image image, vk::Format format, uint32_t mipLevels)
		{
			vk::ImageViewCreateInfo viewInfo = {};
			viewInfo.image = image;
			viewInfo.viewType = vk::ImageViewType::eCube;
			viewInfo.format = format;
			viewInfo.components = { vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA };
			viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = mipLevels;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			// 6 array layers (faces)
			viewInfo.subresourceRange.layerCount = 6;

			vk::ImageView imageView = VKDevice::Instance()->GetDevice().createImageView(viewInfo);

			return imageView;
		}

		void VKTextureCube::UpdateDescriptor()
		{
			m_Descriptor.sampler = m_TextureSampler;
			m_Descriptor.imageView = m_TextureImageView;
			m_Descriptor.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
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

			VKBuffer* stagingBuffer = lmnew VKBuffer(vk::BufferUsageFlagBits::eTransferSrc, static_cast<u32>(size), allData);

			if (m_Data == nullptr)
			{
				lmdel[] allData;
				allData = nullptr;
			}

			CreateImage(faceWidths[0], faceHeights[0], m_NumMips, vk::Format::eR8G8B8A8Unorm, vk::ImageTiling::eOptimal, /*vk::ImageUsageFlagBits::eTransferSrc|*/ vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal, m_TextureImage, m_TextureImageMemory);

			vk::CommandBuffer cmdBuffer = VKTools::BeginSingleTimeCommands();

			// Setup buffer copy regions for each face including all of it's miplevels
			std::vector<vk::BufferImageCopy> bufferCopyRegions;
			uint32_t offset = 0;

			for (uint32_t face = 0; face < 6; face++)
			{
				for (uint32_t level = 0; level < m_NumMips; level++)
				{
					vk::BufferImageCopy bufferCopyRegion = {};
					bufferCopyRegion.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
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
			vk::ImageSubresourceRange subresourceRange = {};
			subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = m_NumMips;
			subresourceRange.layerCount = 6;

			VKTools::SetImageLayout(
				cmdBuffer,
				m_TextureImage,
				vk::ImageLayout::eUndefined,
				vk::ImageLayout::eTransferDstOptimal,
				subresourceRange);

			// Copy the cube map faces from the staging buffer to the optimal tiled image
			cmdBuffer.copyBufferToImage(stagingBuffer->GetBuffer(), m_TextureImage, vk::ImageLayout::eTransferDstOptimal, bufferCopyRegions);
			// Change texture image layout to shader read after all faces have been copied
			m_ImageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

			VKTools::SetImageLayout(
				cmdBuffer,
				m_TextureImage,
				vk::ImageLayout::eTransferDstOptimal,
				m_ImageLayout,
				subresourceRange);

			VKTools::EndSingleTimeCommands(cmdBuffer);

			CreateTextureSampler();
			m_TextureImageView = CreateImageView(m_TextureImage, vk::Format::eR8G8B8A8Unorm, m_NumMips);

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

			if (allData)
			{
				delete[] allData;
			}
		}

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

		VKTextureDepthArray::VKTextureDepthArray(u32 width, u32 height, u32 count)
			: m_Width(width), m_Height(height), m_Count(count)
		{
			Init();
		}

		VKTextureDepthArray::~VKTextureDepthArray()
		{
            VKDevice::Instance()->GetDevice().destroyImageView(m_TextureImageView);

#ifdef USE_VMA_ALLOCATOR
			vmaDestroyImage(VKDevice::Instance()->GetAllocator(), m_TextureImage, m_Allocation);
#else
			VKDevice::Instance()->GetDevice().destroyImage(m_TextureImage);
            VKDevice::Instance()->GetDevice().freeMemory(m_TextureImageMemory);
#endif
            VKDevice::Instance()->GetDevice().destroySampler(m_TextureSampler);

			for (uint32_t i = 0; i < m_Count; i++)
			{
                VKDevice::Instance()->GetDevice().destroyImageView(m_IndividualImageViews[i]);
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

			vk::MemoryRequirements memRequirements;
			VKDevice::Instance()->GetDevice().getImageMemoryRequirements(image, &memRequirements);

			vk::MemoryAllocateInfo allocInfo = {};
			allocInfo.allocationSize = memRequirements.size;
			allocInfo.memoryTypeIndex = VKTools::FindMemoryType(memRequirements.memoryTypeBits, properties);

			imageMemory = VKDevice::Instance()->GetDevice().allocateMemory(allocInfo);
			VKDevice::Instance()->GetDevice().bindImageMemory(image, imageMemory, 0);
#endif
		}

		void VKTextureDepthArray::CreateImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags)
		{
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
			}

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
