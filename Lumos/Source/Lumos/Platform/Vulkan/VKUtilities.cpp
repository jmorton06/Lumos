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
#include "VKInitialisers.h"

namespace Lumos
{
    namespace Graphics
    {
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

            throw std::runtime_error("Failed to find suitable memory type!");
        }

        void VKUtilities::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer,
            VkDeviceMemory& bufferMemory, VmaAllocator allocator, VmaAllocation allocation)
        {
            VkBufferCreateInfo bufferInfo = {};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = size;
            bufferInfo.usage = usage;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

#ifdef USE_VMA_ALLOCATOR
            if(allocator != nullptr)
            {
                VmaAllocationCreateInfo vmaAllocInfo = {};
                vmaAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
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
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

            VkResult result = vkAllocateMemory(VKDevice::Get().GetDevice(), &allocInfo, nullptr, &bufferMemory);
            if(result != VK_SUCCESS)
            {
                throw std::runtime_error("failed to allocate buffer memory!");
            }

            vkBindBufferMemory(VKDevice::Get().GetDevice(), buffer, bufferMemory, 0);
        }

        VkCommandBuffer VKUtilities::BeginSingleTimeCommands()
        {
            VkCommandBufferAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandPool = VKDevice::Get().GetCommandPool()->GetHandle();
            allocInfo.commandBufferCount = 1;

            VkCommandBuffer commandBuffer;
            vkAllocateCommandBuffers(VKDevice::Get().GetDevice(), &allocInfo, &commandBuffer);

            VkCommandBufferBeginInfo beginInfo = {};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &beginInfo));

            return commandBuffer;
        }

        void VKUtilities::EndSingleTimeCommands(VkCommandBuffer commandBuffer)
        {
            VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

            VkSubmitInfo submitInfo;
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffer;
            submitInfo.pSignalSemaphores = nullptr;
            submitInfo.pNext = nullptr;
            submitInfo.pWaitDstStageMask = nullptr;
            submitInfo.signalSemaphoreCount = 0;
            submitInfo.waitSemaphoreCount = 0;

            VK_CHECK_RESULT(vkQueueSubmit(VKDevice::Get().GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE));
            VK_CHECK_RESULT(vkQueueWaitIdle(VKDevice::Get().GetGraphicsQueue()));

            vkFreeCommandBuffers(VKDevice::Get().GetDevice(),
                VKDevice::Get().GetCommandPool()->GetHandle(), 1, &commandBuffer);
        }

        void VKUtilities::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
        {
            VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

            VkBufferImageCopy region;
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = 1;
            region.imageOffset = { 0, 0, 0 };
            region.imageExtent = {
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
            copyRegion.size = size;
            vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

            VKUtilities::EndSingleTimeCommands(commandBuffer);
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
                LUMOS_ASSERT(AccessFlag != 0 && (AccessFlag & (AccessFlag - 1)) == 0, "Error");
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
                    break;

                case VK_ACCESS_MEMORY_WRITE_BIT:
                    break;

                default:
                    LUMOS_LOG_ERROR("Unknown access flag");
                    break;
                }
            }
            return stages;
        }

        VkPipelineStageFlags LayoutToAccessMask(const VkImageLayout layout, const bool isDestination)
        {
            VkPipelineStageFlags accessMask = 0;

            switch(layout)
            {
            case VK_IMAGE_LAYOUT_UNDEFINED:
                if(isDestination)
                {
                    LUMOS_LOG_ERROR("The new layout used in a transition must not be VK_IMAGE_LAYOUT_UNDEFINED.");
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
                    LUMOS_LOG_ERROR("The new layout used in a transition must not be VK_IMAGE_LAYOUT_PREINITIALIZED.");
                }
                break;

            case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:
                accessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
                break;

            case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:
                accessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
                break;

            case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
                accessMask = VK_ACCESS_MEMORY_READ_BIT;
                break;

            default:
                LUMOS_LOG_ERROR("Unexpected image layout");
                break;
            }

            return accessMask;
        }

        bool IsDepthFormat(VkFormat format)
        {
            switch(format)
            {
            case VK_FORMAT_D32_SFLOAT:
            case VK_FORMAT_D32_SFLOAT_S8_UINT:
            case VK_FORMAT_D24_UNORM_S8_UINT:
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
                commandBuffer = BeginSingleTimeCommands();
                singleTimeCommand = true;
            }

            VkImageSubresourceRange subresourceRange = {};
            subresourceRange.aspectMask = IsDepthFormat(format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

            if(IsStencilFormat(format))
                subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;

            subresourceRange.baseMipLevel = 0;
            subresourceRange.levelCount = mipLevels;
            subresourceRange.baseArrayLayer = 0;
            subresourceRange.layerCount = layerCount;

            // Create an image barrier object
            VkImageMemoryBarrier imageMemoryBarrier = VKInitialisers::imageMemoryBarrier();
            imageMemoryBarrier.oldLayout = oldImageLayout;
            imageMemoryBarrier.newLayout = newImageLayout;
            imageMemoryBarrier.image = image;
            imageMemoryBarrier.subresourceRange = subresourceRange;
            imageMemoryBarrier.srcAccessMask = LayoutToAccessMask(oldImageLayout, false);
            imageMemoryBarrier.dstAccessMask = LayoutToAccessMask(newImageLayout, true);
            imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

            VkPipelineStageFlags sourceStage = 0;
            {
                if(imageMemoryBarrier.oldLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
                {
                    sourceStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
                }
                else if(imageMemoryBarrier.srcAccessMask != 0)
                {
                    sourceStage = AccessFlagsToPipelineStage(imageMemoryBarrier.srcAccessMask, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
                }
                else
                {
                    sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
                }
            }

            VkPipelineStageFlags destinationStage = 0;
            {
                if(imageMemoryBarrier.newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
                {
                    destinationStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                }
                else if(imageMemoryBarrier.dstAccessMask != 0)
                {
                    destinationStage = AccessFlagsToPipelineStage(imageMemoryBarrier.dstAccessMask, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
                }
                else
                {
                    destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
                }
            }

            // Put barrier inside setup command buffer
            vkCmdPipelineBarrier(
                commandBuffer,
                //destinationStage, //Causes many warnings - not supported by stage mask, in vulkan validation
                //sourceStage,
                VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &imageMemoryBarrier);

            if(singleTimeCommand)
                EndSingleTimeCommands(commandBuffer);
        }

        VkFormat VKUtilities::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling,
            VkFormatFeatureFlags features)
        {
            for(VkFormat format : candidates)
            {
                VkFormatProperties props;
                vkGetPhysicalDeviceFormatProperties(VKDevice::Get().GetGPU(), format, &props);

                if(tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
                {
                    return format;
                }
                else if(tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
                {
                    return format;
                }
            }

            throw std::runtime_error("failed to find supported format!");
        }

        VkFormat VKUtilities::FindDepthFormat()
        {
            return FindSupportedFormat(
                { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
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
#undef STR
            default:
                return "UNKNOWN_ERROR";
            }
        }

        VkFormat VKUtilities::FormatToVK(Lumos::Graphics::Format format)
        {
            switch(format)
            {
            case Lumos::Graphics::Format::R32G32B32A32_FLOAT:
                return VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
            case Lumos::Graphics::Format::R32G32B32_FLOAT:
                return VkFormat::VK_FORMAT_R32G32B32_SFLOAT;
            case Lumos::Graphics::Format::R32G32_FLOAT:
                return VkFormat::VK_FORMAT_R32G32_SFLOAT;
            case Lumos::Graphics::Format::R32_FLOAT:
                return VkFormat::VK_FORMAT_R32_SFLOAT;
            case Lumos::Graphics::Format::R32G32B32A32_UINT:
                return VkFormat::VK_FORMAT_R32G32B32A32_UINT;
            case Lumos::Graphics::Format::R32G32B32_UINT:
                return VkFormat::VK_FORMAT_R32G32B32_UINT;
            case Lumos::Graphics::Format::R32G32_UINT:
                return VkFormat::VK_FORMAT_R32G32_UINT;
            case Lumos::Graphics::Format::R32_UINT:
                return VkFormat::VK_FORMAT_R32_UINT;
            case Lumos::Graphics::Format::R8_UINT:
                return VkFormat::VK_FORMAT_R8_UINT;
            case Lumos::Graphics::Format::R32G32B32A32_INT:
                return VkFormat::VK_FORMAT_R32G32B32A32_SINT;
            case Lumos::Graphics::Format::R32G32B32_INT:
                return VkFormat::VK_FORMAT_R32G32B32_SINT;
            case Lumos::Graphics::Format::R32G32_INT:
                return VkFormat::VK_FORMAT_R32G32_SINT;
            case Lumos::Graphics::Format::R32_INT:
                return VkFormat::VK_FORMAT_R32_SINT;
            default:
                return VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
            }
        }

        VkVertexInputAttributeDescription VKUtilities::VertexInputDescriptionToVK(VertexInputDescription description)
        {
            VkVertexInputAttributeDescription vInputAttribDescription;
            vInputAttribDescription.location = description.location;
            vInputAttribDescription.binding = description.binding;
            vInputAttribDescription.format = FormatToVK(description.format);
            vInputAttribDescription.offset = description.offset;
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
            }

            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        }

        void VKUtilities::ValidateResolution(uint32_t& width, uint32_t& height)
        {
            VkSurfaceCapabilitiesKHR capabilities;
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VKDevice::Get().GetGPU(), ((VKSwapChain*)Application::Get().GetWindow()->GetSwapChain().get())->GetSurface(), &capabilities);

            width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, width));
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
                LUMOS_LOG_CRITICAL("[Texture] Unsupported wrap type!");
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
                LUMOS_LOG_CRITICAL("[Texture] Unsupported TextureFilter type!");
                return VK_FILTER_LINEAR;
            }
        }

        VkFormat VKUtilities::TextureFormatToVK(const TextureFormat format, bool srgb)
        {
            if(srgb)
            {
                switch(format)
                {
                case TextureFormat::RGBA:
                    return VK_FORMAT_R8G8B8A8_UNORM;
                case TextureFormat::RGB:
                    return VK_FORMAT_R8G8B8_SRGB;
                case TextureFormat::R8:
                    return VK_FORMAT_R8_SRGB;
                case TextureFormat::RG8:
                    return VK_FORMAT_R8G8_SRGB;
                case TextureFormat::RGB8:
                    return VK_FORMAT_R8G8B8A8_SRGB;
                case TextureFormat::RGBA8:
                    return VK_FORMAT_R8G8B8A8_SRGB;
                case TextureFormat::RGB16:
                    return VK_FORMAT_R16G16B16_SFLOAT;
                case TextureFormat::RGBA16:
                    return VK_FORMAT_R16G16B16A16_SFLOAT;
                case TextureFormat::RGB32:
                    return VK_FORMAT_R32G32B32_SFLOAT;
                case TextureFormat::RGBA32:
                    return VK_FORMAT_R32G32B32A32_SFLOAT;
                default:
                    LUMOS_LOG_CRITICAL("[Texture] Unsupported image bit-depth!");
                    return VK_FORMAT_R8G8B8A8_SRGB;
                }
            }
            else
            {
                switch(format)
                {
                case TextureFormat::RGBA:
                    return VK_FORMAT_R8G8B8A8_UNORM;
                case TextureFormat::RGB:
                    return VK_FORMAT_R8G8B8_UNORM;
                case TextureFormat::R8:
                    return VK_FORMAT_R8_UNORM;
                case TextureFormat::RG8:
                    return VK_FORMAT_R8G8_UNORM;
                case TextureFormat::RGB8:
                    return VK_FORMAT_R8G8B8A8_UNORM;
                case TextureFormat::RGBA8:
                    return VK_FORMAT_R8G8B8A8_UNORM;
                case TextureFormat::RGB16:
                    return VK_FORMAT_R16G16B16_SFLOAT;
                case TextureFormat::RGBA16:
                    return VK_FORMAT_R16G16B16A16_SFLOAT;
                case TextureFormat::RGB32:
                    return VK_FORMAT_R32G32B32_SFLOAT;
                case TextureFormat::RGBA32:
                    return VK_FORMAT_R32G32B32A32_SFLOAT;
                default:
                    LUMOS_LOG_CRITICAL("[Texture] Unsupported image bit-depth!");
                    return VK_FORMAT_R8G8B8A8_UNORM;
                }
            }
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
                LUMOS_LOG_CRITICAL("Unknown Shader Type");
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
                LUMOS_LOG_CRITICAL("Unknown Polygon Mode");
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
                LUMOS_LOG_CRITICAL("Unknown Draw Type");
                return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
                break;
            }
        }

        bool VKUtilities::IsPresentModeSupported(const std::vector<VkPresentModeKHR>& supportedModes, VkPresentModeKHR presentMode)
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

        VkPresentModeKHR VKUtilities::ChoosePresentMode(const std::vector<VkPresentModeKHR>& supportedModes, bool vsync)
        {
            VkPresentModeKHR presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
            if(vsync)
            {
                if(IsPresentModeSupported(supportedModes, VK_PRESENT_MODE_MAILBOX_KHR))
                {
                    presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                }
                else if(IsPresentModeSupported(supportedModes, VK_PRESENT_MODE_FIFO_KHR))
                {
                    presentMode = VK_PRESENT_MODE_FIFO_KHR;
                }
                else if(IsPresentModeSupported(supportedModes, VK_PRESENT_MODE_IMMEDIATE_KHR))
                {
                    presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
                }
                else
                {
                    LUMOS_LOG_ERROR("Failed to find supported presentation mode.");
                }
            }
            else
            {
                if(IsPresentModeSupported(supportedModes, VK_PRESENT_MODE_IMMEDIATE_KHR))
                {
                    presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
                }
                else if(IsPresentModeSupported(supportedModes, VK_PRESENT_MODE_FIFO_RELAXED_KHR))
                {
                    presentMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
                }
                else if(IsPresentModeSupported(supportedModes, VK_PRESENT_MODE_FIFO_KHR))
                {
                    presentMode = VK_PRESENT_MODE_FIFO_KHR;
                }
                else
                {
                    LUMOS_LOG_ERROR("Failed to find supported presentation mode.");
                }
            }

            return presentMode;
        }

        void VKUtilities::WaitIdle()
        {
            LUMOS_PROFILE_FUNCTION();
            vkDeviceWaitIdle(VKDevice::GetHandle());
        }
    }
}
