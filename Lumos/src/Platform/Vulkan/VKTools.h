#pragma once
#include "VK.h"
#include <vulkan/vk_mem_alloc.h>

#define VK_CHECK_RESULT(f)																				\
{																										\
	VkResult res = (f);																					\
	if (res != VK_SUCCESS)																				\
	{																									\
		Lumos::Debug::Log::Error("[VULKAN] : VkResult is {0} in {1} at line {2}",Lumos::Graphics::VKTools::ErrorString(res) , __FILE__ , __LINE__); \
	}																									\
}

namespace Lumos
{
	namespace Graphics
	{
		struct VertexInputDescription;
		enum class CullMode;
		enum class DescriptorType;
        enum class ShaderType : int;
		enum class TextureFormat;
		enum class TextureWrap;
        enum class Format;
        enum class TextureFilter;
        
		namespace VKTools
		{
			void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer,
				VkDeviceMemory& bufferMemory, VmaAllocator allocator = nullptr, VmaAllocation allocation = nullptr);

			VkCommandBuffer BeginSingleTimeCommands();
			void EndSingleTimeCommands(VkCommandBuffer commandBuffer);

			void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
			void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

			bool HasStencilComponent(VkFormat format);

			void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels = 1);

			uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
			VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
			VkFormat FindDepthFormat();

			std::string ErrorString(VkResult errorCode);

			VkVertexInputAttributeDescription VertexInputDescriptionToVK(VertexInputDescription description);
			VkCullModeFlags CullModeToVK(CullMode mode);
			VkDescriptorType DescriptorTypeToVK(DescriptorType type);
			VkFormat TextureFormatToVK(const TextureFormat format, bool srgb = true);
			VkSamplerAddressMode TextureWrapToVK(const TextureWrap format);
            VkFilter TextureFilterToVK(const TextureFilter filter);
            VkShaderStageFlagBits ShaderTypeToVK(const ShaderType& shaderName);
            VkFormat FormatToVK(Lumos::Graphics::Format format);

			void SetImageLayout(
				VkCommandBuffer cmdbuffer,
				VkImage image,
				VkImageLayout oldImageLayout,
				VkImageLayout newImageLayout,
				VkImageSubresourceRange subresourceRange,
				VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
			// Uses a fixed sub resource layout with first mip level and layer
			void SetImageLayout(
				VkCommandBuffer cmdbuffer,
				VkImage image,
				VkImageAspectFlags aspectMask,
				VkImageLayout oldImageLayout,
				VkImageLayout newImageLayout,
				VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
		}
	}
}
