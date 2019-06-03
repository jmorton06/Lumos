#include "LM.h"
#include "VKTexture2D.h"
#include "VKDevice.h"
#include "Utilities/LoadImage.h"
#include "VKTools.h"
#include "VKCommandBuffer.h"

#include "Maths/MathsUtilities.h"

#include <cmath>

namespace Lumos
{
	namespace Graphics
	{
		VKTexture2D::VKTexture2D(uint width, uint height, void* data, TextureParameters parameters, TextureLoadOptions loadOptions)
			: m_FileName("NULL"), m_TextureSampler(nullptr), m_TextureImageView(nullptr)
		{
			m_Width = width;
			m_Height = height;
			m_Parameters = parameters;
			m_LoadOptions = loadOptions;
			m_Data = static_cast<byte*>(data);
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
				VKDevice::Instance()->GetDevice().destroyImage(m_TextureImage);
				if (m_TextureImageMemory)
				{
					VKDevice::Instance()->GetDevice().freeMemory(m_TextureImageMemory);
				}
			}
		}

		void VKTexture2D::Bind(uint slot) const
		{
		}

		void VKTexture2D::Unbind(uint slot) const
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
			default: LUMOS_CORE_ERROR("[Texture] Unsupported wrap type!");  return vk::SamplerAddressMode::eClampToEdge;
			}
		}

		void VKTexture2D::BuildTexture(TextureFormat internalformat, uint width, uint height, bool depth, bool samplerShadow)
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

			image = VKDevice::Instance()->GetDevice().createImage(imageInfo);

            vk::MemoryRequirements memRequirements;
            VKDevice::Instance()->GetDevice().getImageMemoryRequirements(image, &memRequirements);

            vk::MemoryAllocateInfo allocInfo = {};
			allocInfo.allocationSize = memRequirements.size;
			allocInfo.memoryTypeIndex = VKTools::FindMemoryType(memRequirements.memoryTypeBits, properties);

			imageMemory = VKDevice::Instance()->GetDevice().allocateMemory(allocInfo);
			VKDevice::Instance()->GetDevice().bindImageMemory(image, imageMemory, 0);
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
			uint texWidth, texHeight, texChannels;
			byte* pixels;

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
                LUMOS_CORE_ERROR("failed to load texture image!");
			}

			m_MipLevels = static_cast<uint32_t>(std::floor(std::log2(Maths::Max(texWidth, texHeight)))) + 1;


			vk::Buffer stagingBuffer;
			vk::DeviceMemory stagingBufferMemory;
			VKTools::CreateBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc,
				vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer,
				stagingBufferMemory);

			void* data;
			VKDevice::Instance()->GetDevice().mapMemory(stagingBufferMemory, vk::DeviceSize(0), imageSize, vk::MemoryMapFlagBits(), &data);
			memcpy(data, pixels, static_cast<size_t>(imageSize));
			VKDevice::Instance()->GetDevice().unmapMemory(stagingBufferMemory);

			if (m_Data == nullptr)
				delete[] pixels;

            CreateImage(texWidth, texHeight, m_MipLevels, vk::Format::eR8G8B8A8Unorm, vk::ImageTiling::eOptimal,  vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal, m_TextureImage, m_TextureImageMemory);

			VKTools::TransitionImageLayout(m_TextureImage, vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eUndefined,
				vk::ImageLayout::eTransferDstOptimal, m_MipLevels);
			VKTools::CopyBufferToImage(stagingBuffer, m_TextureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
			
			VKDevice::Instance()->GetDevice().destroyBuffer(stagingBuffer);
			VKDevice::Instance()->GetDevice().freeMemory(stagingBufferMemory);

			GenerateMipmaps(m_TextureImage, texWidth, texHeight, m_MipLevels);

			return true;
		}
	}
}
