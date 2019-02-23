#include "LM.h"
#include "VKTools.h"
#include "Graphics/API/DescriptorSet.h"
#include "Graphics/API/Pipeline.h"
#include "Platform/Vulkan/VKCommandBuffer.h"

namespace Lumos
{
	namespace graphics
	{
		namespace VKTools
		{
            uint32_t FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
			{
                vk::PhysicalDeviceMemoryProperties memProperties = VKDevice::Instance()->GetGPU().getMemoryProperties();

				for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
				{
					if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
					{
						return i;
					}
				}

				throw std::runtime_error("failed to find suitable memory type!");
			}

			void CreateBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Buffer& buffer,
			                  vk::DeviceMemory& bufferMemory)
			{
				vk::BufferCreateInfo bufferInfo = {};
				bufferInfo.size = size;
				bufferInfo.usage = usage;
				bufferInfo.sharingMode = vk::SharingMode::eExclusive;

				buffer = VKDevice::Instance()->GetDevice().createBuffer(bufferInfo);

				vk::MemoryRequirements memRequirements = VKDevice::Instance()->GetDevice().getBufferMemoryRequirements(buffer);

				vk::MemoryAllocateInfo allocInfo = {};
				allocInfo.allocationSize = memRequirements.size;
				allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);
				
				vk::Result result = VKDevice::Instance()->GetDevice().allocateMemory(&allocInfo, nullptr, &bufferMemory);
				if (result != vk::Result::eSuccess)
				{					
					throw std::runtime_error("failed to allocate buffer memory!");
				}

				VKDevice::Instance()->GetDevice().bindBufferMemory(buffer,bufferMemory, 0);
			}

			vk::CommandBuffer BeginSingleTimeCommands()
			{
				vk::CommandBufferAllocateInfo allocInfo = {};
				allocInfo.level = vk::CommandBufferLevel::ePrimary;
				allocInfo.commandPool = VKDevice::Instance()->GetVKContext()->GetCommandPool()->GetCommandPool();
				allocInfo.commandBufferCount = 1;

				vk::CommandBuffer commandBuffer = VKDevice::Instance()->GetDevice().allocateCommandBuffers(allocInfo)[0];

				vk::CommandBufferBeginInfo beginInfo = {};
				beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

				commandBuffer.begin(beginInfo);

				return commandBuffer;
			}

			void EndSingleTimeCommands(vk::CommandBuffer commandBuffer)
			{
				vkEndCommandBuffer(commandBuffer);

				vk::SubmitInfo submitInfo = {};
				submitInfo.commandBufferCount = 1;
				submitInfo.pCommandBuffers = &commandBuffer;

				VKDevice::Instance()->GetGraphicsQueue().submit(1, &submitInfo, nullptr);
				VKDevice::Instance()->GetGraphicsQueue().waitIdle();

				VKDevice::Instance()->GetDevice().freeCommandBuffers(VKDevice::Instance()->GetVKContext()->GetCommandPool()->GetCommandPool(), 1, &commandBuffer);
			}

			void CopyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height)
			{
				vk::CommandBuffer commandBuffer = BeginSingleTimeCommands();

				vk::BufferImageCopy region = {};
				region.bufferOffset = 0;
				region.bufferRowLength = 0;
				region.bufferImageHeight = 0;
				region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
				region.imageSubresource.mipLevel = 0;
				region.imageSubresource.baseArrayLayer = 0;
				region.imageSubresource.layerCount = 1;
                region.imageOffset = vk::Offset3D(0, 0, 0);
                region.imageExtent = vk::Extent3D(width, height, 1);

				commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, 1, &region);

				VKTools::EndSingleTimeCommands(commandBuffer);
			}

			void CopyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size)
			{
				vk::CommandBuffer commandBuffer = VKTools::BeginSingleTimeCommands();

				vk::BufferCopy copyRegion = {};
				copyRegion.size = size;
				commandBuffer.copyBuffer(srcBuffer, dstBuffer, copyRegion);

				VKTools::EndSingleTimeCommands(commandBuffer);
			}

			bool HasStencilComponent(vk::Format format)
			{
				return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
			}

			void TransitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
				uint32_t mipLevels)
			{
				vk::CommandBuffer commandBuffer = BeginSingleTimeCommands();

				vk::ImageMemoryBarrier barrier = {};
				barrier.oldLayout = oldLayout;
				barrier.newLayout = newLayout;
				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.image = image;

				if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
				{
					barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;

					if (HasStencilComponent(format))
					{
						barrier.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
					}
				}
				else
				{
					barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
				}

				barrier.subresourceRange.baseMipLevel = 0;
				barrier.subresourceRange.levelCount = mipLevels;
				barrier.subresourceRange.baseArrayLayer = 0;
				barrier.subresourceRange.layerCount = 1;

				vk::PipelineStageFlags sourceStage;
				vk::PipelineStageFlags destinationStage;

				if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
				{
					barrier.srcAccessMask = vk::AccessFlagBits(0);
					barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

					sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
					destinationStage = vk::PipelineStageFlagBits::eTransfer;
				}
				else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
				{
					barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
					barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

					sourceStage = vk::PipelineStageFlagBits::eTransfer;
					destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
				}
				else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
				{
					barrier.srcAccessMask = vk::AccessFlagBits(0);
					barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;

					sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
					destinationStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
				}
				else
				{
					LUMOS_CORE_ERROR("unsupported layout transition!");
				}

				commandBuffer.pipelineBarrier(sourceStage, destinationStage, static_cast<vk::DependencyFlagBits>(0), 0, nullptr, 0, nullptr, 1, &barrier);

				EndSingleTimeCommands(commandBuffer);
			}

			vk::Format FindSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling,
											vk::FormatFeatureFlags features)
			{
				for (vk::Format format : candidates)
				{
					vk::FormatProperties props = VKDevice::Instance()->GetGPU().getFormatProperties(format);

					if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features)
					{
						return format;
					}
					else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features)
					{
						return format;
					}
				}

				throw std::runtime_error("failed to find supported format!");
			}

			vk::Format FindDepthFormat()
			{
				return FindSupportedFormat(
					{ vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint,vk::Format::eD24UnormS8Uint},
					vk::ImageTiling::eOptimal,
					vk::FormatFeatureFlagBits::eDepthStencilAttachment
				);
			}

			std::string errorString(VkResult errorCode)
			{
				switch (errorCode)
				{
#define STR(r) case VK_ ##r: return #r
					STR(NOT_READY);
					STR(TIMEOUT);
					STR(EVENT_SET);
					STR(EVENT_RESET);
					STR(INCOMPLETE);
					STR(ERROR_OUT_OF_HOST_MEMORY);
					STR(ERROR_OUT_OF_DEVICE_MEMORY);
					STR(ERROR_INITIALIZATION_FAILED);
					STR(ERROR_DEVICE_LOST);
					STR(ERROR_MEMORY_MAP_FAILED);
					STR(ERROR_LAYER_NOT_PRESENT);
					STR(ERROR_EXTENSION_NOT_PRESENT);
					STR(ERROR_FEATURE_NOT_PRESENT);
					STR(ERROR_INCOMPATIBLE_DRIVER);
					STR(ERROR_TOO_MANY_OBJECTS);
					STR(ERROR_FORMAT_NOT_SUPPORTED);
					STR(ERROR_SURFACE_LOST_KHR);
					STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
					STR(SUBOPTIMAL_KHR);
					STR(ERROR_OUT_OF_DATE_KHR);
					STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
					STR(ERROR_VALIDATION_FAILED_EXT);
					STR(ERROR_INVALID_SHADER_NV);
#undef STR
				default:
					return "UNKNOWN_ERROR";
				}
			}

			vk::Format FormatToVK(api::Format format)
			{
				switch(format)
				{
				case api::R32G32B32A32_FLOAT : return vk::Format::eR32G32B32A32Sfloat;
				case api::R32G32B32_FLOAT	 : return vk::Format::eR32G32B32Sfloat;
				case api::R32G32_FLOAT		 : return vk::Format::eR32G32Sfloat;
				default: return vk::Format::eR32G32B32Sfloat;
				}
			}

			vk::VertexInputAttributeDescription VertexInputDescriptionToVK(api::VertexInputDescription description)
			{
				vk::VertexInputAttributeDescription vInputAttribDescription{};
				vInputAttribDescription.location = description.location;
				vInputAttribDescription.binding = description.binding;
				vInputAttribDescription.format = FormatToVK(description.format);
				vInputAttribDescription.offset = description.offset;
				return vInputAttribDescription;
			}

			vk::CullModeFlags CullModeToVK(api::CullMode mode)
			{
				switch(mode)
				{
				case api::CullMode::BACK		 : return vk::CullModeFlagBits::eBack;
				case api::CullMode::FRONT		 : return vk::CullModeFlagBits::eFront;
				case api::CullMode::FRONTANDBACK : return vk::CullModeFlagBits::eFrontAndBack;
				case api::CullMode::NONE		 : return vk::CullModeFlagBits::eNone;

				}

				return vk::CullModeFlagBits::eBack;
			}

			vk::DescriptorType DescriptorTypeToVK(api::DescriptorType type)
			{
				switch (type)
				{
				case api::DescriptorType::UNIFORM_BUFFER		 : return vk::DescriptorType::eUniformBuffer;
				case api::DescriptorType::UNIFORM_BUFFER_DYNAMIC : return vk::DescriptorType::eUniformBufferDynamic;
				case api::DescriptorType::IMAGE_SAMPLER			 : return vk::DescriptorType::eCombinedImageSampler;
				}

				return vk::DescriptorType::eUniformBuffer;
			}

			vk::ShaderStageFlags ShaderStageToVK(api::ShaderStage type)
			{
				switch (type)
				{
				case api::ShaderStage::VERTEX   : return vk::ShaderStageFlagBits::eVertex;
				case api::ShaderStage::FRAGMENT : return vk::ShaderStageFlagBits::eFragment;
				case api::ShaderStage::GEOMETRY : return vk::ShaderStageFlagBits::eGeometry;
				}

				return vk::ShaderStageFlagBits::eVertex;
			}

			void SetImageLayout(
				vk::CommandBuffer cmdbuffer,
				vk::Image image,
				vk::ImageLayout oldImageLayout,
				vk::ImageLayout newImageLayout,
				vk::ImageSubresourceRange subresourceRange,
				vk::PipelineStageFlags srcStageMask,
				vk::PipelineStageFlags dstStageMask)
			{
				// Create an image barrier object
				vk::ImageMemoryBarrier imageMemoryBarrier;
				imageMemoryBarrier.oldLayout = oldImageLayout;
				imageMemoryBarrier.newLayout = newImageLayout;
				imageMemoryBarrier.image = image;
				imageMemoryBarrier.subresourceRange = subresourceRange;

				// Source layouts (old)
				// Source access mask controls actions that have to be finished on the old layout
				// before it will be transitioned to the new layout
				switch (oldImageLayout)
				{
				case vk::ImageLayout::eUndefined:
					// Image layout is undefined (or does not matter)
					// Only valid as initial layout
					// No flags required, listed only for completeness
					//imageMemoryBarrier.srcAccessMask = 0;
					break;

				case vk::ImageLayout::ePreinitialized:
					// Image is preinitialized
					// Only valid as initial layout for linear images, preserves memory contents
					// Make sure host writes have been finished
					imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eHostWrite;
					break;

				case vk::ImageLayout::eColorAttachmentOptimal:
					// Image is a color attachment
					// Make sure any writes to the color buffer have been finished
					imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
					break;

				case vk::ImageLayout::eDepthStencilAttachmentOptimal:
					// Image is a depth/stencil attachment
					// Make sure any writes to the depth/stencil buffer have been finished
					imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
					break;

				case vk::ImageLayout::eTransferSrcOptimal:
					// Image is a transfer source 
					// Make sure any reads from the image have been finished
					imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
					break;

				case vk::ImageLayout::eTransferDstOptimal:
					// Image is a transfer destination
					// Make sure any writes to the image have been finished
					imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
					break;

				case vk::ImageLayout::eShaderReadOnlyOptimal:
					// Image is read by a shader
					// Make sure any shader reads from the image have been finished
					imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eShaderRead;
					break;
				default:
					// Other source layouts aren't handled (yet)
					break;
				}

				// Target layouts (new)
				// Destination access mask controls the dependency for the new image layout
				switch (newImageLayout)
				{
				case vk::ImageLayout::eTransferDstOptimal:
					// Image will be used as a transfer destination
					// Make sure any writes to the image have been finished
					imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
					break;

				case vk::ImageLayout::eTransferSrcOptimal:
					// Image will be used as a transfer source
					// Make sure any reads from the image have been finished
					imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
					break;

				case vk::ImageLayout::eColorAttachmentOptimal:
					// Image will be used as a color attachment
					// Make sure any writes to the color buffer have been finished
					imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
					break;

				case vk::ImageLayout::eDepthStencilAttachmentOptimal:
					// Image layout will be used as a depth/stencil attachment
					// Make sure any writes to depth/stencil buffer have been finished
					imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
					break;

				case vk::ImageLayout::eShaderReadOnlyOptimal:
					// Image will be read in a shader (sampler, input attachment)
					// Make sure any writes to the image have been finished
					if (imageMemoryBarrier.srcAccessMask == vk::AccessFlagBits())
					{
						imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eHostWrite | vk::AccessFlagBits::eTransferWrite;
					}
					imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
					break;
				default:
					// Other source layouts aren't handled (yet)
					break;
				}

				// Put barrier inside setup command buffer
				cmdbuffer.pipelineBarrier(srcStageMask, dstStageMask, static_cast<vk::DependencyFlagBits>(0), 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
			}

			// Fixed sub resource on first mip level and layer
			void SetImageLayout(
				vk::CommandBuffer cmdbuffer,
				vk::Image image,
				vk::ImageAspectFlags aspectMask,
				vk::ImageLayout oldImageLayout,
				vk::ImageLayout newImageLayout,
				vk::PipelineStageFlags srcStageMask,
				vk::PipelineStageFlags dstStageMask)
			{
				vk::ImageSubresourceRange subresourceRange = {};
				subresourceRange.aspectMask = aspectMask;
				subresourceRange.baseMipLevel = 0;
				subresourceRange.levelCount = 1;
				subresourceRange.layerCount = 1;
				SetImageLayout(cmdbuffer, image, oldImageLayout, newImageLayout, subresourceRange, srcStageMask, dstStageMask);
			}

			VkShaderModule LoadShader(const char *fileName, VkDevice device)
			{
				std::ifstream is(fileName, std::ios::binary | std::ios::in | std::ios::ate);

				if (is.is_open())
				{
					size_t size = is.tellg();
					is.seekg(0, std::ios::beg);
					char* shaderCode = new char[size];
					is.read(shaderCode, size);
					is.close();

					assert(size > 0);

					VkShaderModule shaderModule;
					VkShaderModuleCreateInfo moduleCreateInfo{};
					moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
					moduleCreateInfo.codeSize = size;
					moduleCreateInfo.pCode = (uint32_t*)shaderCode;

					vkCreateShaderModule(device, &moduleCreateInfo, NULL, &shaderModule);

					delete[] shaderCode;

					return shaderModule;
				}
				else
				{
					std::cerr << "Error: Could not open shader file \"" << fileName << "\"" << std::endl;
					return VK_NULL_HANDLE;
				}
			}
			VkPipelineShaderStageCreateInfo loadShader(std::string fileName, VkShaderStageFlagBits stage)
			{
				VkPipelineShaderStageCreateInfo shaderStage = {};
				shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				shaderStage.stage = stage;
				shaderStage.module = LoadShader(fileName.c_str(), VKDevice::Instance()->GetDevice());
				shaderStage.pName = "main"; // todo : make param
				assert(shaderStage.module != VK_NULL_HANDLE);

				return shaderStage;
			}
		}
	}
}
