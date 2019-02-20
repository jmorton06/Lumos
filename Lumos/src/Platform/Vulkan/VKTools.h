#pragma once
#include "VKDevice.h"
#include "VKCommandPool.h"

#include <cassert>

#define VK_CHECK_RESULT(f)																				\
{																										\
	VkResult res = (f);																					\
	if (res != VK_SUCCESS)																				\
	{																									\
		LUMOS_CORE_ERROR("[VULKAN] : VkResult is {0} in {1} at line {2}",Lumos::graphics::VKTools::errorString(res) , __FILE__ , __LINE__); \
		assert(res == VK_SUCCESS);																		\
	}																									\
}

namespace Lumos
{
	namespace graphics
	{
		class VKCommandBuffer;

		namespace api
		{
			struct VertexInputDescription;
			enum class CullMode;
			enum class DescriptorType;
			enum class ShaderStage;
		}

		namespace VKTools
		{
            uint32_t FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);

			void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer,
			                  VkDeviceMemory& bufferMemory);

			void endSingleTimeCommands(VkCommandBuffer commandBuffer);

			VkCommandBuffer beginSingleTimeCommands();

			void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
			void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

			bool hasStencilComponent(VkFormat format);

			void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
			void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);

			vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling,
			                             vk::FormatFeatureFlags features);

			vk::Format findDepthFormat();

			std::string errorString(VkResult errorCode);

			vk::VertexInputAttributeDescription VertexInputDescriptionToVK(api::VertexInputDescription description);
			vk::CullModeFlags CullModeToVK(api::CullMode mode);
			vk::DescriptorType DescriptorTypeToVK(api::DescriptorType type);
			vk::ShaderStageFlags ShaderStageToVK(api::ShaderStage type);
			vk::Format FormatToVK(api::Format format);

			void setImageLayout(
				VkCommandBuffer cmdbuffer,
				VkImage image,
				VkImageLayout oldImageLayout,
				VkImageLayout newImageLayout,
				VkImageSubresourceRange subresourceRange,
				VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
			// Uses a fixed sub resource layout with first mip level and layer
			void setImageLayout(
				VkCommandBuffer cmdbuffer,
				VkImage image,
				VkImageAspectFlags aspectMask,
				VkImageLayout oldImageLayout,
				VkImageLayout newImageLayout,
				VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

				VkPipelineShaderStageCreateInfo loadShader(std::string fileName, VkShaderStageFlagBits stage);
		}
	}
}
