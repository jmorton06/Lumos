#pragma once
#include "VK.h"
#include "Graphics/RHI/RHIDefinitions.h"

#define VK_CHECK_RESULT(f)                                                                                                                    \
    {                                                                                                                                         \
        VkResult res = (f);                                                                                                                   \
        if(res != VK_SUCCESS)                                                                                                                 \
        {                                                                                                                                     \
            LERROR("[VULKAN] : VkResult is %s in %s at line %i", Lumos::Graphics::VKUtilities::ErrorString(res).c_str(), __FILE__, __LINE__); \
        }                                                                                                                                     \
    }

#define VK_CHECK_RESULT_RETURN_FALSE(f)                                                                                                       \
    {                                                                                                                                         \
        VkResult res = (f);                                                                                                                   \
        if(res != VK_SUCCESS)                                                                                                                 \
        {                                                                                                                                     \
            LERROR("[VULKAN] : VkResult is %s in %s at line %i", Lumos::Graphics::VKUtilities::ErrorString(res).c_str(), __FILE__, __LINE__); \
            return false;                                                                                                                     \
        }                                                                                                                                     \
    }

namespace Lumos
{
    namespace Graphics
    {
        namespace VKUtilities
        {
            void Init();
#ifdef USE_VMA_ALLOCATOR
            void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer,
                              VkDeviceMemory& bufferMemory, VmaAllocator allocator = nullptr, VmaAllocation allocation = nullptr);
#else
            void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer,
                          VkDeviceMemory& bufferMemory);
#endif
            VkCommandBuffer BeginSingleTimeCommands();
            void EndSingleTimeCommands(VkCommandBuffer commandBuffer);

            void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
            void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

            void InsertImageMemoryBarrier(
                VkCommandBuffer cmdbuffer,
                VkImage image,
                VkAccessFlags srcAccessMask,
                VkAccessFlags dstAccessMask,
                VkImageLayout oldImageLayout,
                VkImageLayout newImageLayout,
                VkPipelineStageFlags srcStageMask,
                VkPipelineStageFlags dstStageMask,
                VkImageSubresourceRange subresourceRange);

            bool HasStencilComponent(VkFormat format);

            void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels = 1, uint32_t layerCount = 1, VkCommandBuffer commandBuffer = nullptr);

            uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
            VkFormat FindSupportedFormat(const TDArray<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
            VkFormat FindDepthFormat();

            void ValidateResolution(uint32_t& width, uint32_t& height);
            bool IsPresentModeSupported(const TDArray<VkPresentModeKHR>& supportedModes, VkPresentModeKHR presentMode);
            VkPresentModeKHR ChoosePresentMode(const TDArray<VkPresentModeKHR>& supportedModes, bool vsync);

            void WaitIdle();
            std::string ErrorString(VkResult errorCode);

            VkVertexInputAttributeDescription VertexInputDescriptionToVK(VertexInputDescription description);
            VkCullModeFlags CullModeToVK(CullMode mode);
            VkDescriptorType DescriptorTypeToVK(DescriptorType type);
            VkFormat FormatToVK(const RHIFormat format, bool srgb = false);
            RHIFormat VKToFormat(VkFormat format);

            uint32_t BytesPerPixel(VkFormat format);

            VkSamplerAddressMode TextureWrapToVK(const TextureWrap format);
            VkFilter TextureFilterToVK(const TextureFilter filter);
            VkShaderStageFlagBits ShaderTypeToVK(const ShaderType& shaderName);
            VkPolygonMode PolygonModeToVk(Lumos::Graphics::PolygonMode mode);
            VkPrimitiveTopology DrawTypeToVk(Lumos::Graphics::DrawType type);

            void SetDebugUtilsObjectName(const VkDevice device, const VkObjectType objectType, const char* name, const void* handle);

            VkSampleCountFlagBits GetMaxUsableSampleCount();
        }
    }
}
