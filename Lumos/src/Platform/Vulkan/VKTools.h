#pragma once
#include "VK.h"

#define VK_CHECK_RESULT(f)																				\
{																										\
	VkResult res = (f);																					\
	if (res != VK_SUCCESS)																				\
	{																									\
		LUMOS_CORE_ERROR("[VULKAN] : VkResult is {0} in {1} at line {2}",lumos::graphics::VKTools::errorString(res) , __FILE__ , __LINE__); \
		assert(res == VK_SUCCESS);																		\
	}																									\
}

namespace lumos
{
	namespace graphics
	{
		struct VertexInputDescription;
		enum class CullMode;
		enum class DescriptorType;
		enum class ShaderStage;
		enum class TextureFormat;

		namespace VKTools
		{
			void CreateBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Buffer& buffer,
				vk::DeviceMemory& bufferMemory);

			vk::CommandBuffer BeginSingleTimeCommands();
			void EndSingleTimeCommands(vk::CommandBuffer commandBuffer);

			void CopyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height);
			void CopyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size);

			bool HasStencilComponent(vk::Format format);

			void TransitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevels = 1);

			uint32_t FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
			vk::Format FindSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);
			vk::Format FindDepthFormat();

			std::string ErrorString(VkResult errorCode);

			vk::VertexInputAttributeDescription VertexInputDescriptionToVK(VertexInputDescription description);
			vk::CullModeFlags CullModeToVK(CullMode mode);
			vk::DescriptorType DescriptorTypeToVK(DescriptorType type);
			vk::ShaderStageFlags ShaderStageToVK(ShaderStage type);
			vk::Format TextureFormatToVK(const TextureFormat format);

			void SetImageLayout(
				vk::CommandBuffer cmdbuffer,
				vk::Image image,
				vk::ImageLayout oldImageLayout,
				vk::ImageLayout newImageLayout,
				vk::ImageSubresourceRange subresourceRange,
				vk::PipelineStageFlags srcStageMask = vk::PipelineStageFlagBits::eAllCommands,
				vk::PipelineStageFlags dstStageMask = vk::PipelineStageFlagBits::eAllCommands);
			// Uses a fixed sub resource layout with first mip level and layer
			void SetImageLayout(
				vk::CommandBuffer cmdbuffer,
				vk::Image image,
				vk::ImageAspectFlags aspectMask,
				vk::ImageLayout oldImageLayout,
				vk::ImageLayout newImageLayout,
				vk::PipelineStageFlags srcStageMask = vk::PipelineStageFlagBits::eAllCommands,
				vk::PipelineStageFlags dstStageMask = vk::PipelineStageFlagBits::eAllCommands);
		}
	}
}
