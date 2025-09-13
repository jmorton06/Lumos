#include "Precompiled.h"
#include "VKUtilities.h"
#include "VKDevice.h"
#include "VKShader.h"
#include "VKSwapChain.h"
#include "VKCommandPool.h"
#include "Graphics/RHI/Texture.h"
#include "Graphics/RHI/DescriptorSet.h"
#include "Graphics/RHI/Pipeline.h"
#include "Core/Application.h"
#include "Core/OS/Window.h"
#include "VKInitialisers.h"
#include "vulkan/vulkan_core.h"

namespace Lumos
{
    namespace Graphics
    {
        void VKUtilities::Init()
        {
            fpSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)(vkGetInstanceProcAddr(VKContext::GetVKInstance(), "vkSetDebugUtilsObjectNameEXT"));
            if(fpSetDebugUtilsObjectNameEXT == nullptr)
                fpSetDebugUtilsObjectNameEXT = [](VkDevice device, const VkDebugUtilsObjectNameInfoEXT* pNameInfo)
                { return VK_SUCCESS; };

            fpCmdBeginDebugUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT)(vkGetInstanceProcAddr(VKContext::GetVKInstance(), "vkCmdBeginDebugUtilsLabelEXT"));
            if(fpCmdBeginDebugUtilsLabelEXT == nullptr)
                fpCmdBeginDebugUtilsLabelEXT = [](VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT* pLabelInfo) { };

            fpCmdEndDebugUtilsLabelEXT = (PFN_vkCmdEndDebugUtilsLabelEXT)(vkGetInstanceProcAddr(VKContext::GetVKInstance(), "vkCmdEndDebugUtilsLabelEXT"));
            if(fpCmdEndDebugUtilsLabelEXT == nullptr)
                fpCmdEndDebugUtilsLabelEXT = [](VkCommandBuffer commandBuffer) { };
        }

        uint32_t VKUtilities::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
        {
            VkPhysicalDeviceMemoryProperties memProperties;
            vkGetPhysicalDeviceMemoryProperties(VKDevice::Get().GetGPU(), &memProperties);

            for(uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
            {
                if((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
                {
                    return i;
                }
            }

            LERROR("Failed to find suitable memory type!");
            return 0;
        }

        void VKUtilities::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer,
                                       VkDeviceMemory& bufferMemory, VmaAllocator allocator, VmaAllocation allocation)
        {
            VkBufferCreateInfo bufferInfo = {};
            bufferInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size               = size;
            bufferInfo.usage              = usage;
            bufferInfo.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;

#ifdef USE_VMA_ALLOCATOR
            if(allocator != nullptr)
            {
                VmaAllocationCreateInfo vmaAllocInfo = {};
                vmaAllocInfo.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;
                vmaCreateBuffer(allocator, &bufferInfo, &vmaAllocInfo, &buffer, &allocation, nullptr);
            }
            else
            {
                vkCreateBuffer(VKDevice::Get().GetDevice(), &bufferInfo, nullptr, &buffer);
            }
#else
            vkCreateBuffer(VKDevice::Get().GetDevice(), &bufferInfo, nullptr, &buffer);
#endif
            VkMemoryRequirements memRequirements;
            vkGetBufferMemoryRequirements(VKDevice::Get().GetDevice(), buffer, &memRequirements);

            VkMemoryAllocateInfo allocInfo = {};
            allocInfo.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize       = memRequirements.size;
            allocInfo.memoryTypeIndex      = FindMemoryType(memRequirements.memoryTypeBits, properties);

            VkResult result = vkAllocateMemory(VKDevice::Get().GetDevice(), &allocInfo, nullptr, &bufferMemory);
            if(result != VK_SUCCESS)
            {
                LERROR("Failed to allocate buffer memory!");
            }

            vkBindBufferMemory(VKDevice::Get().GetDevice(), buffer, bufferMemory, 0);
        }

        VkCommandBuffer VKUtilities::BeginSingleTimeCommands()
        {
            VkCommandBufferAllocateInfo allocInfo = {};
            allocInfo.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandPool                 = VKDevice::Get().GetCommandPool()->GetHandle();
            allocInfo.commandBufferCount          = 1;

            VkCommandBuffer commandBuffer;
            vkAllocateCommandBuffers(VKDevice::Get().GetDevice(), &allocInfo, &commandBuffer);

            VkCommandBufferBeginInfo beginInfo = {};
            beginInfo.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &beginInfo));

            return commandBuffer;
        }

        void VKUtilities::EndSingleTimeCommands(VkCommandBuffer commandBuffer)
        {
            VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

            VkFence fence;
            VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
            vkCreateFence(VKDevice::Get().GetDevice(), &fenceInfo, nullptr, &fence);
            
            VkSubmitInfo submitInfo;
            submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount   = 1;
            submitInfo.pCommandBuffers      = &commandBuffer;
            submitInfo.pSignalSemaphores    = nullptr;
            submitInfo.pNext                = nullptr;
            submitInfo.pWaitDstStageMask    = nullptr;
            submitInfo.signalSemaphoreCount = 0;
            submitInfo.waitSemaphoreCount   = 0;

            VK_CHECK_RESULT(vkQueueSubmit(VKDevice::Get().GetGraphicsQueue(), 1, &submitInfo, fence));

            VK_CHECK_RESULT(vkWaitForFences(VKDevice::Get().GetDevice(), 1, &fence, VK_TRUE, UINT64_MAX));
            vkDestroyFence(VKDevice::Get().GetDevice(), fence, nullptr);
            
            vkFreeCommandBuffers(VKDevice::Get().GetDevice(),
                                 VKDevice::Get().GetCommandPool()->GetHandle(), 1, &commandBuffer);
        }

        void VKUtilities::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
        {
            VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

            VkBufferImageCopy region;
            region.bufferOffset                    = 0;
            region.bufferRowLength                 = 0;
            region.bufferImageHeight               = 0;
            region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel       = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount     = 1;
            region.imageOffset                     = { 0, 0, 0 };
            region.imageExtent                     = {
                width,
                height,
                1
            };

            vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

            VKUtilities::EndSingleTimeCommands(commandBuffer);
        }

        void VKUtilities::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
        {
            VkCommandBuffer commandBuffer = VKUtilities::BeginSingleTimeCommands();

            VkBufferCopy copyRegion = {};
            copyRegion.size         = size;
            vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

            VKUtilities::EndSingleTimeCommands(commandBuffer);
        }

        void VKUtilities::InsertImageMemoryBarrier(
            VkCommandBuffer cmdbuffer,
            VkImage image,
            VkAccessFlags srcAccessMask,
            VkAccessFlags dstAccessMask,
            VkImageLayout oldImageLayout,
            VkImageLayout newImageLayout,
            VkPipelineStageFlags srcStageMask,
            VkPipelineStageFlags dstStageMask,
            VkImageSubresourceRange subresourceRange)
        {
            VkImageMemoryBarrier imageMemoryBarrier = VKInitialisers::ImageMemoryBarrier();
            imageMemoryBarrier.srcAccessMask        = srcAccessMask;
            imageMemoryBarrier.dstAccessMask        = dstAccessMask;
            imageMemoryBarrier.oldLayout            = oldImageLayout;
            imageMemoryBarrier.newLayout            = newImageLayout;
            imageMemoryBarrier.image                = image;
            imageMemoryBarrier.subresourceRange     = subresourceRange;

            vkCmdPipelineBarrier(
                cmdbuffer,
                srcStageMask,
                dstStageMask,
                0,
                0, nullptr,
                0, nullptr,
                1, &imageMemoryBarrier);
        }

        bool VKUtilities::HasStencilComponent(VkFormat format)
        {
            return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
        }

        VkPipelineStageFlags AccessFlagsToPipelineStage(VkAccessFlags accessFlags, const VkPipelineStageFlags stageFlags)
        {
            VkPipelineStageFlags stages = 0;

            while(accessFlags != 0)
            {
                VkAccessFlagBits AccessFlag = static_cast<VkAccessFlagBits>(accessFlags & (~(accessFlags - 1)));
                ASSERT(AccessFlag != 0 && (AccessFlag & (AccessFlag - 1)) == 0, "Error");
                accessFlags &= ~AccessFlag;

                switch(AccessFlag)
                {
                case VK_ACCESS_INDIRECT_COMMAND_READ_BIT:
                    stages |= VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
                    break;

                case VK_ACCESS_INDEX_READ_BIT:
                    stages |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
                    break;

                case VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT:
                    stages |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
                    break;

                case VK_ACCESS_UNIFORM_READ_BIT:
                    stages |= stageFlags | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
                    break;

                case VK_ACCESS_INPUT_ATTACHMENT_READ_BIT:
                    stages |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                    break;

                case VK_ACCESS_SHADER_READ_BIT:
                    stages |= stageFlags | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
                    break;

                case VK_ACCESS_SHADER_WRITE_BIT:
                    stages |= stageFlags | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
                    break;

                case VK_ACCESS_COLOR_ATTACHMENT_READ_BIT:
                    stages |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                    break;

                case VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT:
                    stages |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                    break;

                case VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT:
                    stages |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                    break;

                case VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT:
                    stages |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                    break;

                case VK_ACCESS_TRANSFER_READ_BIT:
                    stages |= VK_PIPELINE_STAGE_TRANSFER_BIT;
                    break;

                case VK_ACCESS_TRANSFER_WRITE_BIT:
                    stages |= VK_PIPELINE_STAGE_TRANSFER_BIT;
                    break;

                case VK_ACCESS_HOST_READ_BIT:
                    stages |= VK_PIPELINE_STAGE_HOST_BIT;
                    break;

                case VK_ACCESS_HOST_WRITE_BIT:
                    stages |= VK_PIPELINE_STAGE_HOST_BIT;
                    break;

                case VK_ACCESS_MEMORY_READ_BIT:
                case VK_ACCESS_MEMORY_WRITE_BIT:
                default:
                    LERROR("Unknown access flag");
                    stages |= VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
                    break;
                }
            }
            return stages;
        }

        VkAccessFlags LayoutToAccessMask(const VkImageLayout layout, const bool isDestination)
        {
            VkAccessFlags accessMask = 0;

            switch(layout)
            {
            case VK_IMAGE_LAYOUT_UNDEFINED:
                if(isDestination)
                {
                    LERROR("The new layout used in a transition must not be VK_IMAGE_LAYOUT_UNDEFINED.");
                }
                break;

            case VK_IMAGE_LAYOUT_GENERAL:
                accessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                accessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                accessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
                accessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
                break;

            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                accessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
                break;

            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                accessMask = VK_ACCESS_TRANSFER_READ_BIT;
                break;

            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                accessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                break;

            case VK_IMAGE_LAYOUT_PREINITIALIZED:
                if(!isDestination)
                {
                    accessMask = VK_ACCESS_HOST_WRITE_BIT;
                }
                else
                {
                    LERROR("The new layout used in a transition must not be VK_IMAGE_LAYOUT_PREINITIALIZED.");
                }
                break;

            case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:
                accessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
                break;

            case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:
                accessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
                break;

            case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
                accessMask = 0;
                break;

            default:
                LERROR("Unexpected image layout");
                break;
            }

            return accessMask;
        }

        bool IsDepthFormat(VkFormat format)
        {
            switch(format)
            {
            case VK_FORMAT_D16_UNORM:
            case VK_FORMAT_D32_SFLOAT:
            case VK_FORMAT_D32_SFLOAT_S8_UINT:
            case VK_FORMAT_D24_UNORM_S8_UINT:
            case VK_FORMAT_D16_UNORM_S8_UINT:
                return true;
            }
            return false;
        }

        bool IsStencilFormat(VkFormat format)
        {
            switch(format)
            {
            case VK_FORMAT_D32_SFLOAT_S8_UINT:
            case VK_FORMAT_D24_UNORM_S8_UINT:
                return true;
            }
            return false;
        }

        void VKUtilities::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldImageLayout, VkImageLayout newImageLayout,
                                                uint32_t mipLevels, uint32_t layerCount, VkCommandBuffer commandBuffer)
        {
            LUMOS_PROFILE_FUNCTION();

            bool singleTimeCommand = false;

            if(!commandBuffer)
            {
                commandBuffer     = BeginSingleTimeCommands();
                singleTimeCommand = true;
            }

            VkImageSubresourceRange subresourceRange = {};
            subresourceRange.aspectMask              = IsDepthFormat(format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

            if(IsStencilFormat(format))
                subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;

            subresourceRange.baseMipLevel   = 0;
            subresourceRange.levelCount     = mipLevels;
            subresourceRange.baseArrayLayer = 0;
            subresourceRange.layerCount     = layerCount;

            // Create an image barrier object
            VkImageMemoryBarrier imageMemoryBarrier = VKInitialisers::ImageMemoryBarrier();
            imageMemoryBarrier.oldLayout            = oldImageLayout;
            imageMemoryBarrier.newLayout            = newImageLayout;
            imageMemoryBarrier.image                = image;
            imageMemoryBarrier.subresourceRange     = subresourceRange;
            imageMemoryBarrier.srcAccessMask        = LayoutToAccessMask(oldImageLayout, false);
            imageMemoryBarrier.dstAccessMask        = LayoutToAccessMask(newImageLayout, true);
            imageMemoryBarrier.srcQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.dstQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;

            VkPipelineStageFlags sourceStage = 0;
            {
                if(imageMemoryBarrier.oldLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
                {
                    sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                }
                else if(imageMemoryBarrier.srcAccessMask != 0)
                {
                    sourceStage = AccessFlagsToPipelineStage(imageMemoryBarrier.srcAccessMask, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
                }
                else
                {
                    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                }
            }

            VkPipelineStageFlags destinationStage = 0;
            {
                if(imageMemoryBarrier.newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
                {
                    destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                }
                else if(imageMemoryBarrier.dstAccessMask != 0)
                {
                    destinationStage = AccessFlagsToPipelineStage(imageMemoryBarrier.dstAccessMask, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
                }
                else
                {
                    destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
                }
            }

            // Put barrier inside setup command buffer
            vkCmdPipelineBarrier(
                commandBuffer,
                sourceStage,
                destinationStage,
                0,
                0, nullptr,
                0, nullptr,
                1, &imageMemoryBarrier);

            if(singleTimeCommand)
                EndSingleTimeCommands(commandBuffer);
        }

        VkFormat VKUtilities::FindSupportedFormat(const TDArray<VkFormat>& candidates, VkImageTiling tiling,
                                                  VkFormatFeatureFlags features)
        {
            for(VkFormat format : candidates)
            {
                VkFormatProperties props;
                vkGetPhysicalDeviceFormatProperties(VKDevice::Get().GetGPU(), format, &props);

                if(!(props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT))
                {
                    continue;
                }

                if(tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
                {
                    return format;
                }
                else if(tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
                {
                    return format;
                }
            }

            LERROR("Failed to find supported format!");
            return VK_FORMAT_UNDEFINED;
        }

        VkFormat VKUtilities::FindDepthFormat()
        {
            return FindSupportedFormat(
                { VK_FORMAT_D32_SFLOAT_S8_UINT,
                  VK_FORMAT_D32_SFLOAT,
                  VK_FORMAT_D24_UNORM_S8_UINT,
                  VK_FORMAT_D16_UNORM_S8_UINT,
                  VK_FORMAT_D16_UNORM },
                VK_IMAGE_TILING_OPTIMAL,
                VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
        }

        std::string VKUtilities::ErrorString(VkResult errorCode)
        {
            switch(errorCode)
            {
#define STR(r)   \
    case VK_##r: \
        return #r
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
                STR(SUCCESS);
                STR(ERROR_FRAGMENTED_POOL);
                STR(ERROR_OUT_OF_POOL_MEMORY);
                STR(ERROR_INVALID_EXTERNAL_HANDLE);
                STR(ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT);
                STR(ERROR_FRAGMENTATION_EXT);
                STR(ERROR_NOT_PERMITTED_EXT);
                STR(ERROR_INVALID_DEVICE_ADDRESS_EXT);
                STR(ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT);
                STR(ERROR_UNKNOWN);
                STR(RESULT_MAX_ENUM);
#undef STR
            default:
                return "UNKNOWN_ERROR";
            }
        }

        VkVertexInputAttributeDescription VKUtilities::VertexInputDescriptionToVK(VertexInputDescription description)
        {
            VkVertexInputAttributeDescription vInputAttribDescription;
            vInputAttribDescription.location = description.location;
            vInputAttribDescription.binding  = description.binding;
            vInputAttribDescription.format   = FormatToVK(description.format);
            vInputAttribDescription.offset   = description.offset;
            return vInputAttribDescription;
        }

        VkCullModeFlags VKUtilities::CullModeToVK(CullMode mode)
        {
            switch(mode)
            {
            case CullMode::BACK:
                return VK_CULL_MODE_BACK_BIT;
            case CullMode::FRONT:
                return VK_CULL_MODE_FRONT_BIT;
            case CullMode::FRONTANDBACK:
                return VK_CULL_MODE_FRONT_AND_BACK;
            case CullMode::NONE:
                return VK_CULL_MODE_NONE;
            }

            return VK_CULL_MODE_BACK_BIT;
        }

        VkDescriptorType VKUtilities::DescriptorTypeToVK(DescriptorType type)
        {
            switch(type)
            {
            case DescriptorType::UNIFORM_BUFFER:
                return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            case DescriptorType::UNIFORM_BUFFER_DYNAMIC:
                return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            case DescriptorType::IMAGE_SAMPLER:
                return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            case DescriptorType::IMAGE_STORAGE:
                return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            }

            LINFO("Unsupported Descriptor Type");
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        }

        void VKUtilities::ValidateResolution(uint32_t& width, uint32_t& height)
        {
            VkSurfaceCapabilitiesKHR capabilities;
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VKDevice::Get().GetGPU(), ((VKSwapChain*)Application::Get().GetWindow()->GetSwapChain().get())->GetSurface(), &capabilities);

            width  = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, width));
            height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, height));
        }

        VkSamplerAddressMode VKUtilities::TextureWrapToVK(const TextureWrap wrap)
        {
            switch(wrap)
            {
            case TextureWrap::CLAMP:
                return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            case TextureWrap::CLAMP_TO_BORDER:
                return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            case TextureWrap::CLAMP_TO_EDGE:
                return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            case TextureWrap::REPEAT:
                return VK_SAMPLER_ADDRESS_MODE_REPEAT;
            case TextureWrap::MIRRORED_REPEAT:
                return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            default:
                LFATAL("[Texture] Unsupported wrap type!");
                return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            }
        }

        VkFilter VKUtilities::TextureFilterToVK(const TextureFilter filter)
        {
            switch(filter)
            {
            case TextureFilter::NEAREST:
                return VK_FILTER_NEAREST;
            case TextureFilter::LINEAR:
                return VK_FILTER_LINEAR;
            case TextureFilter::NONE:
                return VK_FILTER_LINEAR;
            default:
                LFATAL("[Texture] Unsupported TextureFilter type!");
                return VK_FILTER_LINEAR;
            }
        }

        VkFormat VKUtilities::FormatToVK(const RHIFormat format, bool srgb)
        {
            if(srgb)
            {
                switch(format)
                {
                case RHIFormat::R8_Unorm:
                    return VK_FORMAT_R8_SRGB;
                case RHIFormat::R8G8_Unorm:
                    return VK_FORMAT_R8G8_SRGB;
                case RHIFormat::R8G8B8_Unorm:
                    return VK_FORMAT_R8G8B8_SRGB;
                case RHIFormat::R8G8B8A8_Unorm:
                    return VK_FORMAT_R8G8B8A8_SRGB;
                case RHIFormat::R16G16B16_Float:
                    return VK_FORMAT_R16G16B16_SFLOAT;
                case RHIFormat::R16G16B16A16_Float:
                    return VK_FORMAT_R16G16B16A16_SFLOAT;
                case RHIFormat::R32G32B32_Float:
                    return VK_FORMAT_R32G32B32_SFLOAT;
                case RHIFormat::R32G32B32A32_Float:
                    return VK_FORMAT_R32G32B32A32_SFLOAT;
                default:
                    LFATAL("[Texture] Unsupported image bit-depth!");
                    return VK_FORMAT_R8G8B8A8_SRGB;
                }
            }
            else
            {
                switch(format)
                {
                case RHIFormat::R8_Unorm:
                    return VK_FORMAT_R8_UNORM;
                case RHIFormat::R8G8_Unorm:
                    return VK_FORMAT_R8G8_UNORM;
                case RHIFormat::R8G8B8_Unorm:
                    return VK_FORMAT_R8G8B8A8_UNORM;
                case RHIFormat::R8G8B8A8_Unorm:
                    return VK_FORMAT_R8G8B8A8_UNORM;
                case RHIFormat::R11G11B10_Float:
                    return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
                case RHIFormat::R10G10B10A2_Unorm:
                    return VK_FORMAT_A2R10G10B10_UNORM_PACK32;
                case RHIFormat::R16_Float:
                    return VK_FORMAT_R16_SFLOAT;
                case RHIFormat::R16G16_Float:
                    return VK_FORMAT_R16G16_SFLOAT;
                case RHIFormat::R16G16B16_Float:
                    return VK_FORMAT_R16G16B16_SFLOAT;
                case RHIFormat::R16G16B16A16_Float:
                    return VK_FORMAT_R16G16B16A16_SFLOAT;
                case RHIFormat::R32_Float:
                    return VK_FORMAT_R32_SFLOAT;
                case RHIFormat::R32G32_Float:
                    return VK_FORMAT_R32G32_SFLOAT;
                case RHIFormat::R32G32B32_Float:
                    return VK_FORMAT_R32G32B32_SFLOAT;
                case RHIFormat::R32G32B32A32_Float:
                    return VK_FORMAT_R32G32B32A32_SFLOAT;
                case RHIFormat::D16_Unorm:
                    return VK_FORMAT_D16_UNORM;
                case RHIFormat::D32_Float:
                    return VK_FORMAT_D32_SFLOAT;
                case RHIFormat::D24_Unorm_S8_UInt:
                    return VK_FORMAT_D24_UNORM_S8_UINT;
                case RHIFormat::D32_Float_S8_UInt:
                    return VK_FORMAT_D32_SFLOAT_S8_UINT;
                default:
                    LFATAL("[Texture] Unsupported image bit-depth!");
                    return VK_FORMAT_R8G8B8A8_UNORM;
                }
            }
        }

        RHIFormat VKUtilities::VKToFormat(VkFormat format)
        {
            switch(format)
            {
            case VK_FORMAT_R8_SRGB:
                return RHIFormat::R8_Unorm;
            case VK_FORMAT_R8G8_SRGB:
                return RHIFormat::R8G8_Unorm;
            case VK_FORMAT_R8G8B8_SRGB:
                return RHIFormat::R8G8B8_Unorm;
            case VK_FORMAT_R8G8B8A8_SRGB:
                return RHIFormat::R8G8B8A8_Unorm;
            case VK_FORMAT_R16G16B16_SFLOAT:
                return RHIFormat::R16G16B16_Float;
            case VK_FORMAT_R16G16B16A16_SFLOAT:
                return RHIFormat::R16G16B16A16_Float;
            case VK_FORMAT_R32G32B32_SFLOAT:
                return RHIFormat::R32G32B32_Float;
            case VK_FORMAT_R32G32B32A32_SFLOAT:
                return RHIFormat::R32G32B32A32_Float;
            case VK_FORMAT_R8_UNORM:
                return RHIFormat::R8_Unorm;
            case VK_FORMAT_R8G8_UNORM:
                return RHIFormat::R8G8_Unorm;
            case VK_FORMAT_R8G8B8A8_UNORM:
                return RHIFormat::R8G8B8A8_Unorm;
            case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
                return RHIFormat::R11G11B10_Float;
            case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
                return RHIFormat::R10G10B10A2_Unorm;
            case VK_FORMAT_D16_UNORM:
                return RHIFormat::D16_Unorm;
            case VK_FORMAT_D32_SFLOAT:
                return RHIFormat::D32_Float;
            case VK_FORMAT_D24_UNORM_S8_UINT:
                return RHIFormat::D24_Unorm_S8_UInt;
            case VK_FORMAT_D32_SFLOAT_S8_UINT:
                return RHIFormat::D32_Float_S8_UInt;
            default:
                LFATAL("[Texture] Unsupported texture type!");
                return RHIFormat::R8G8B8A8_Unorm;
            }
        }

        uint32_t VKUtilities::BytesPerPixel(VkFormat format)
        {
            switch(format)
            {
            case VK_FORMAT_R8_UNORM:
            case VK_FORMAT_R8_UINT:
                return 1;
            case VK_FORMAT_R5G6B5_UNORM_PACK16:
            case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
            case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
            case VK_FORMAT_R16_SFLOAT:
            case VK_FORMAT_R8G8_SNORM:
            case VK_FORMAT_R8G8_UINT:
            case VK_FORMAT_R16_UINT:
            case VK_FORMAT_D16_UNORM:
                return 2;
            case VK_FORMAT_D16_UNORM_S8_UINT:
                return 3;
            case VK_FORMAT_R8G8B8A8_UNORM:
            case VK_FORMAT_B8G8R8A8_UNORM:
            case VK_FORMAT_R32_SFLOAT:
            case VK_FORMAT_R16G16_UNORM:
            case VK_FORMAT_R16G16_SFLOAT:
            case VK_FORMAT_R8G8B8A8_SNORM:
            case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
            case VK_FORMAT_R8G8B8A8_UINT:
            case VK_FORMAT_R16G16_UINT:
            case VK_FORMAT_D32_SFLOAT:
            case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
                return 4;
            case VK_FORMAT_D32_SFLOAT_S8_UINT:
                return 5;
            case VK_FORMAT_R16G16B16A16_SFLOAT:
            case VK_FORMAT_R16G16B16A16_UNORM:
            case VK_FORMAT_R32G32_SFLOAT:
            case VK_FORMAT_R16G16B16A16_UINT:
            case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
                return 8;
            case VK_FORMAT_R32G32B32A32_SFLOAT:
            case VK_FORMAT_BC2_UNORM_BLOCK:
            case VK_FORMAT_BC3_UNORM_BLOCK:
            case VK_FORMAT_BC7_UNORM_BLOCK:
                return 16;
            default:
                LERROR("Unsupported Vulkan format");
                return 4;
            }

            return 4;
        }

        VkShaderStageFlagBits VKUtilities::ShaderTypeToVK(const ShaderType& shaderName)
        {
            switch(shaderName)
            {
            case ShaderType::VERTEX:
                return VK_SHADER_STAGE_VERTEX_BIT;
            case ShaderType::GEOMETRY:
                return VK_SHADER_STAGE_GEOMETRY_BIT;
            case ShaderType::FRAGMENT:
                return VK_SHADER_STAGE_FRAGMENT_BIT;
            case ShaderType::TESSELLATION_CONTROL:
                return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
            case ShaderType::TESSELLATION_EVALUATION:
                return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
            case ShaderType::COMPUTE:
                return VK_SHADER_STAGE_COMPUTE_BIT;
            default:
                LFATAL("Unknown Shader Type");
                return VK_SHADER_STAGE_VERTEX_BIT;
            }
        }

        VkPolygonMode VKUtilities::PolygonModeToVk(Lumos::Graphics::PolygonMode mode)
        {
            switch(mode)
            {
            case Graphics::PolygonMode::FILL:
                return VK_POLYGON_MODE_FILL;
                break;
            case Graphics::PolygonMode::LINE:
                return VK_POLYGON_MODE_LINE;
                break;
            case Graphics::PolygonMode::POINT:
                return VK_POLYGON_MODE_POINT;
                break;
            default:
                LFATAL("Unknown Polygon Mode");
                return VK_POLYGON_MODE_FILL;
                break;
            }
        }

        VkPrimitiveTopology VKUtilities::DrawTypeToVk(Lumos::Graphics::DrawType type)
        {
            switch(type)
            {
            case DrawType::TRIANGLE:
                return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
                break;
            case DrawType::LINES:
                return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
                break;
            case DrawType::POINT:
                return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
                break;
            default:
                LFATAL("Unknown Draw Type");
                return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
                break;
            }
        }

        bool VKUtilities::IsPresentModeSupported(const TDArray<VkPresentModeKHR>& supportedModes, VkPresentModeKHR presentMode)
        {
            for(const auto& mode : supportedModes)
            {
                if(mode == presentMode)
                {
                    return true;
                }
            }
            return false;
        }

        VkPresentModeKHR VKUtilities::ChoosePresentMode(const TDArray<VkPresentModeKHR>& supportedModes, bool vsync)
        {
            VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
            if(!vsync)
            {
                if(IsPresentModeSupported(supportedModes, VK_PRESENT_MODE_IMMEDIATE_KHR))
                {
                    presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
                }
                else if(IsPresentModeSupported(supportedModes, VK_PRESENT_MODE_FIFO_KHR))
                {
                    presentMode = VK_PRESENT_MODE_FIFO_KHR;
                }
                else
                {
                    LERROR("Failed to find supported presentation mode.");
                }
            }

            // Modes
            //  if(IsPresentModeSupported(supportedModes, VK_PRESENT_MODE_FIFO_RELAXED_KHR))
            //  {
            //      presentMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
            //  }
            //  else if(IsPresentModeSupported(supportedModes, VK_PRESENT_MODE_MAILBOX_KHR))
            //  {
            //      presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
            //  }

            return presentMode;
        }

        void VKUtilities::WaitIdle()
        {
            LUMOS_PROFILE_FUNCTION();
            vkDeviceWaitIdle(VKDevice::GetHandle());
        }

        void VKUtilities::SetDebugUtilsObjectName(const VkDevice device, const VkObjectType objectType, const char* name, const void* handle)
        {
            VkDebugUtilsObjectNameInfoEXT nameInfo;
            nameInfo.sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
            nameInfo.objectType   = objectType;
            nameInfo.pObjectName  = name;
            nameInfo.objectHandle = (uint64_t)handle;
            nameInfo.pNext        = VK_NULL_HANDLE;

            VK_CHECK_RESULT(fpSetDebugUtilsObjectNameEXT(device, &nameInfo));
        }

        VkSampleCountFlagBits VKUtilities::GetMaxUsableSampleCount()
        {
            VkPhysicalDeviceProperties physicalDeviceProperties;
            vkGetPhysicalDeviceProperties(VKDevice::Get().GetPhysicalDevice()->GetHandle(), &physicalDeviceProperties);

            VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
            if(counts & VK_SAMPLE_COUNT_64_BIT)
            {
                return VK_SAMPLE_COUNT_64_BIT;
            }
            if(counts & VK_SAMPLE_COUNT_32_BIT)
            {
                return VK_SAMPLE_COUNT_32_BIT;
            }
            if(counts & VK_SAMPLE_COUNT_16_BIT)
            {
                return VK_SAMPLE_COUNT_16_BIT;
            }
            if(counts & VK_SAMPLE_COUNT_8_BIT)
            {
                return VK_SAMPLE_COUNT_8_BIT;
            }
            if(counts & VK_SAMPLE_COUNT_4_BIT)
            {
                return VK_SAMPLE_COUNT_4_BIT;
            }
            if(counts & VK_SAMPLE_COUNT_2_BIT)
            {
                return VK_SAMPLE_COUNT_2_BIT;
            }

            return VK_SAMPLE_COUNT_1_BIT;
        }
    }
}
