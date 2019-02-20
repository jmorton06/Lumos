#include "LM.h"
#include "VKTools.h"
#include "Graphics/API/DescriptorSet.h"
#include "VKInitialisers.h"
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

			void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer,
			                  VkDeviceMemory& bufferMemory)
			{
				VkBufferCreateInfo bufferInfo = {};
				bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				bufferInfo.size = size;
				bufferInfo.usage = usage;
				bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

				if (vkCreateBuffer(VKDevice::Instance()->GetDevice(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
				{
					throw std::runtime_error("failed to create buffer!");
				}

				VkMemoryRequirements memRequirements;
				vkGetBufferMemoryRequirements(VKDevice::Instance()->GetDevice(), buffer, &memRequirements);

				VkMemoryAllocateInfo allocInfo = {};
				allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
				allocInfo.allocationSize = memRequirements.size;
				allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);
				
				VkResult result = vkAllocateMemory(VKDevice::Instance()->GetDevice(), &allocInfo, nullptr, &bufferMemory);
				if (result != VK_SUCCESS)
				{					
					throw std::runtime_error("failed to allocate buffer memory!");
				}

				vkBindBufferMemory(VKDevice::Instance()->GetDevice(), buffer, bufferMemory, 0);
			}

			void endSingleTimeCommands(VkCommandBuffer commandBuffer)
			{
				vkEndCommandBuffer(commandBuffer);

				VkSubmitInfo submitInfo = {};
				submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submitInfo.commandBufferCount = 1;
				submitInfo.pCommandBuffers = &commandBuffer;

				vkQueueSubmit(VKDevice::Instance()->GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
				vkQueueWaitIdle(VKDevice::Instance()->GetGraphicsQueue());

				vkFreeCommandBuffers(VKDevice::Instance()->GetDevice(),
				                     VKDevice::Instance()->GetVKContext()->GetCommandPool()->GetCommandPool(), 1, &commandBuffer);
			}

			VkCommandBuffer beginSingleTimeCommands()
			{
				VkCommandBufferAllocateInfo allocInfo = {};
				allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
				allocInfo.commandPool = VKDevice::Instance()->GetVKContext()->GetCommandPool()->GetCommandPool();
				allocInfo.commandBufferCount = 1;

				VkCommandBuffer commandBuffer;
				vkAllocateCommandBuffers(VKDevice::Instance()->GetDevice(), &allocInfo, &commandBuffer);

				VkCommandBufferBeginInfo beginInfo = {};
				beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

				vkBeginCommandBuffer(commandBuffer, &beginInfo);

				return commandBuffer;
			}

			void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
			{
				VkCommandBuffer commandBuffer = beginSingleTimeCommands();

				VkBufferImageCopy region = {};
				region.bufferOffset = 0;
				region.bufferRowLength = 0;
				region.bufferImageHeight = 0;
				region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				region.imageSubresource.mipLevel = 0;
				region.imageSubresource.baseArrayLayer = 0;
				region.imageSubresource.layerCount = 1;
				region.imageOffset = {0, 0, 0};
				region.imageExtent = {
					width,
					height,
					1
				};

				vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

				VKTools::endSingleTimeCommands(commandBuffer);
			}

			void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
			{
				VkCommandBuffer commandBuffer = VKTools::beginSingleTimeCommands();

				VkBufferCopy copyRegion = {};
				copyRegion.size = size;
				vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

				VKTools::endSingleTimeCommands(commandBuffer);
			}

			bool hasStencilComponent(VkFormat format)
			{
				return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
			}

			void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
			{
				VkCommandBuffer commandBuffer = beginSingleTimeCommands();

				VkImageMemoryBarrier barrier = {};
				barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				barrier.oldLayout = oldLayout;
				barrier.newLayout = newLayout;
				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.image = image;

				if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
				{
					barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

					if (hasStencilComponent(format))
					{
						barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
					}
				}
				else
				{
					barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				}

				barrier.subresourceRange.baseMipLevel = 0;
				barrier.subresourceRange.levelCount = 1;
				barrier.subresourceRange.baseArrayLayer = 0;
				barrier.subresourceRange.layerCount = 1;

				VkPipelineStageFlags sourceStage;
				VkPipelineStageFlags destinationStage;

				if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
				{
					barrier.srcAccessMask = 0;
					barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

					sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
					destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				}
				else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
				{
					barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

					sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
					destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				}
				else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
				{
					barrier.srcAccessMask = 0;
					barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

					sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
					destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
				}
				else
				{
					throw std::invalid_argument("unsupported layout transition!");
				}

				vkCmdPipelineBarrier(
					commandBuffer,
					sourceStage, destinationStage,
					0,
					0, nullptr,
					0, nullptr,
					1, &barrier
				);

				endSingleTimeCommands(commandBuffer);
			}

			void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout,
				uint32_t mipLevels)
			{
				VkCommandBuffer commandBuffer = beginSingleTimeCommands();

				VkImageMemoryBarrier barrier = {};
				barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				barrier.oldLayout = oldLayout;
				barrier.newLayout = newLayout;
				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.image = image;

				if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
				{
					barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

					if (hasStencilComponent(format))
					{
						barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
					}
				}
				else
				{
					barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				}

				barrier.subresourceRange.baseMipLevel = 0;
				barrier.subresourceRange.levelCount = mipLevels;
				barrier.subresourceRange.baseArrayLayer = 0;
				barrier.subresourceRange.layerCount = 1;

				VkPipelineStageFlags sourceStage;
				VkPipelineStageFlags destinationStage;

				if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
				{
					barrier.srcAccessMask = 0;
					barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

					sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
					destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				}
				else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
				{
					barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

					sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
					destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				}
				else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
				{
					barrier.srcAccessMask = 0;
					barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

					sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
					destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
				}
				else
				{
					throw std::invalid_argument("unsupported layout transition!");
				}

				vkCmdPipelineBarrier(
					commandBuffer,
					sourceStage, destinationStage,
					0,
					0, nullptr,
					0, nullptr,
					1, &barrier
				);

				endSingleTimeCommands(commandBuffer);
			}

			vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling,
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

			vk::Format findDepthFormat()
			{
				return findSupportedFormat(
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
				case api::R32G32B32A32_FLOAT : return vk::Format::eR32G32B32A32Sfloat;// VK_FORMAT_R32G32B32A32_SFLOAT;
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
				case api::DescriptorType::IMAGE_SAMPLER			 : return  vk::DescriptorType::eCombinedImageSampler;
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

			void setImageLayout(
				VkCommandBuffer cmdbuffer,
				VkImage image,
				VkImageLayout oldImageLayout,
				VkImageLayout newImageLayout,
				VkImageSubresourceRange subresourceRange,
				VkPipelineStageFlags srcStageMask,
				VkPipelineStageFlags dstStageMask)
			{
				// Create an image barrier object
				VkImageMemoryBarrier imageMemoryBarrier = initializers::imageMemoryBarrier();
				imageMemoryBarrier.oldLayout = oldImageLayout;
				imageMemoryBarrier.newLayout = newImageLayout;
				imageMemoryBarrier.image = image;
				imageMemoryBarrier.subresourceRange = subresourceRange;

				// Source layouts (old)
				// Source access mask controls actions that have to be finished on the old layout
				// before it will be transitioned to the new layout
				switch (oldImageLayout)
				{
				case VK_IMAGE_LAYOUT_UNDEFINED:
					// Image layout is undefined (or does not matter)
					// Only valid as initial layout
					// No flags required, listed only for completeness
					imageMemoryBarrier.srcAccessMask = 0;
					break;

				case VK_IMAGE_LAYOUT_PREINITIALIZED:
					// Image is preinitialized
					// Only valid as initial layout for linear images, preserves memory contents
					// Make sure host writes have been finished
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
					break;

				case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
					// Image is a color attachment
					// Make sure any writes to the color buffer have been finished
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
					break;

				case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
					// Image is a depth/stencil attachment
					// Make sure any writes to the depth/stencil buffer have been finished
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
					break;

				case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
					// Image is a transfer source 
					// Make sure any reads from the image have been finished
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
					break;

				case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
					// Image is a transfer destination
					// Make sure any writes to the image have been finished
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					break;

				case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
					// Image is read by a shader
					// Make sure any shader reads from the image have been finished
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
					break;
				default:
					// Other source layouts aren't handled (yet)
					break;
				}

				// Target layouts (new)
				// Destination access mask controls the dependency for the new image layout
				switch (newImageLayout)
				{
				case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
					// Image will be used as a transfer destination
					// Make sure any writes to the image have been finished
					imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					break;

				case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
					// Image will be used as a transfer source
					// Make sure any reads from the image have been finished
					imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
					break;

				case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
					// Image will be used as a color attachment
					// Make sure any writes to the color buffer have been finished
					imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
					break;

				case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
					// Image layout will be used as a depth/stencil attachment
					// Make sure any writes to depth/stencil buffer have been finished
					imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
					break;

				case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
					// Image will be read in a shader (sampler, input attachment)
					// Make sure any writes to the image have been finished
					if (imageMemoryBarrier.srcAccessMask == 0)
					{
						imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
					}
					imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
					break;
				default:
					// Other source layouts aren't handled (yet)
					break;
				}

				// Put barrier inside setup command buffer
				vkCmdPipelineBarrier(
					cmdbuffer,
					srcStageMask,
					dstStageMask,
					0,
					0, nullptr,
					0, nullptr,
					1, &imageMemoryBarrier);
			}

			// Fixed sub resource on first mip level and layer
			void setImageLayout(
				VkCommandBuffer cmdbuffer,
				VkImage image,
				VkImageAspectFlags aspectMask,
				VkImageLayout oldImageLayout,
				VkImageLayout newImageLayout,
				VkPipelineStageFlags srcStageMask,
				VkPipelineStageFlags dstStageMask)
			{
				VkImageSubresourceRange subresourceRange = {};
				subresourceRange.aspectMask = aspectMask;
				subresourceRange.baseMipLevel = 0;
				subresourceRange.levelCount = 1;
				subresourceRange.layerCount = 1;
				setImageLayout(cmdbuffer, image, oldImageLayout, newImageLayout, subresourceRange, srcStageMask, dstStageMask);
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
