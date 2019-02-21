#include "LM.h"
#include "VKTextureCube.h"
#include "VKDevice.h"
#include "Utilities/LoadImage.h"
#include "VKTools.h"
#include "VKCommandBuffer.h"

#include "Maths/MathsUtilities.h"

#include <cmath>
#include "VKInitialisers.h"
#include "VKBuffer.h"

namespace Lumos
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
				VKDevice::Instance()->GetDevice().destroySampler(m_TextureSampler);

			if (m_TextureImageView)
				VKDevice::Instance()->GetDevice().destroyImageView(m_TextureImageView);

			if (m_DeleteImage)
			{
				VKDevice::Instance()->GetDevice().destroyImage(m_TextureImage);
				VKDevice::Instance()->GetDevice().freeMemory(m_TextureImageMemory);
			}
		}

		void VKTextureCube::Bind(uint slot) const
		{
		}

		void VKTextureCube::Unbind(uint slot) const
		{
		}

		vk::Format VKTextureCube::TextureFormatToVK(const TextureFormat format)
		{
			switch (format)
			{
			case TextureFormat::RGBA:				return vk::Format::eR8G8B8A8Unorm;
			case TextureFormat::RGB:				return vk::Format::eR8G8B8Unorm;
			case TextureFormat::R8:				    return vk::Format::eR8Unorm;
			case TextureFormat::RG8:				return vk::Format::eR8G8Unorm;
			case TextureFormat::RGB8:				return vk::Format::eR8G8B8Unorm;
			case TextureFormat::RGBA8:				return vk::Format::eR8G8B8A8Unorm;
			case TextureFormat::LUMINANCE:			return vk::Format::eR8G8B8A8Unorm;
			case TextureFormat::LUMINANCE_ALPHA:	return vk::Format::eR8G8B8A8Unorm;
			case TextureFormat::RGB16: 				return vk::Format::eR16G16B16A16Unorm;
			default: LUMOS_CORE_ERROR("[Texture] Unsupported image bit-depth!");  return vk::Format::eR8G8B8A8Unorm;
			}
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

			image = VKDevice::Instance()->GetDevice().createImage(imageInfo);

			vk::MemoryRequirements memRequirements;
			VKDevice::Instance()->GetDevice().getImageMemoryRequirements(image, &memRequirements);

			vk::MemoryAllocateInfo allocInfo = {};
			allocInfo.allocationSize = memRequirements.size;
			allocInfo.memoryTypeIndex = VKTools::FindMemoryType(memRequirements.memoryTypeBits, properties);

			imageMemory = VKDevice::Instance()->GetDevice().allocateMemory(allocInfo);
			VKDevice::Instance()->GetDevice().bindImageMemory(image, imageMemory, 0);
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
				byte* data = Lumos::LoadImageFromFile(m_Files[m], &srcWidth, &srcHeight, &bits, !m_LoadOptions.flipY);
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
			vk::Buffer stagingBuffer;
			vk::DeviceMemory stagingBufferMemory;
			VKTools::CreateBuffer(size, vk::BufferUsageFlagBits::eTransferSrc,
				vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer,
				 stagingBufferMemory);


			void* data;
			VKDevice::Instance()->GetDevice().mapMemory(stagingBufferMemory, vk::DeviceSize(0), size, vk::MemoryMapFlagBits(), &data);
			memcpy(data, allData, static_cast<size_t>(size));
			VKDevice::Instance()->GetDevice().unmapMemory(stagingBufferMemory);

			if (m_Data == nullptr)
				delete[] allData;

			CreateImage(faceWidths[0], faceHeights[0], m_NumMips, vk::Format::eR8G8B8A8Unorm, vk::ImageTiling::eOptimal, /*vk::ImageUsageFlagBits::eTransferSrc|*/ vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal, m_TextureImage, m_TextureImageMemory);

			vk::CommandBuffer cmdBuffer = VKTools::beginSingleTimeCommands();

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
			cmdBuffer.copyBufferToImage(stagingBuffer, m_TextureImage, vk::ImageLayout::eTransferDstOptimal, bufferCopyRegions);
			// Change texture image layout to shader read after all faces have been copied
			m_ImageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			
			VKTools::SetImageLayout(
				cmdBuffer,
				m_TextureImage,
				vk::ImageLayout::eTransferDstOptimal,
				m_ImageLayout,
				subresourceRange);

			VKTools::endSingleTimeCommands(cmdBuffer);

			CreateTextureSampler();
			m_TextureImageView =  CreateImageView(m_TextureImage, vk::Format::eR8G8B8A8Unorm, m_NumMips);

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
