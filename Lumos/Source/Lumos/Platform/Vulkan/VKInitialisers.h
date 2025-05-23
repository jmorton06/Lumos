#pragma once

#include <vector>
#include "vulkan/vulkan.h"

namespace Lumos
{
    namespace VKInitialisers
    {

        inline VkMemoryAllocateInfo MemoryAllocateInfo()
        {
            VkMemoryAllocateInfo memAllocInfo = {};
            memAllocInfo.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            return memAllocInfo;
        }

        inline VkMappedMemoryRange MappedMemoryRange()
        {
            VkMappedMemoryRange mappedMemoryRange = {};
            mappedMemoryRange.sType               = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            return mappedMemoryRange;
        }

        inline VkCommandBufferAllocateInfo CommandBufferAllocateInfo(
            VkCommandPool commandPool,
            VkCommandBufferLevel level,
            uint32_t bufferCount)
        {
            VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
            commandBufferAllocateInfo.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            commandBufferAllocateInfo.commandPool                 = commandPool;
            commandBufferAllocateInfo.level                       = level;
            commandBufferAllocateInfo.commandBufferCount          = bufferCount;
            return commandBufferAllocateInfo;
        }

        inline VkCommandPoolCreateInfo CommandPoolCreateInfo()
        {
            VkCommandPoolCreateInfo cmdPoolCreateInfo = {};
            cmdPoolCreateInfo.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            return cmdPoolCreateInfo;
        }

        inline VkCommandBufferBeginInfo CommandBufferBeginInfo()
        {
            VkCommandBufferBeginInfo cmdBufferBeginInfo = {};
            cmdBufferBeginInfo.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            return cmdBufferBeginInfo;
        }

        inline VkCommandBufferInheritanceInfo CommandBufferInheritanceInfo()
        {
            VkCommandBufferInheritanceInfo cmdBufferInheritanceInfo = {};
            cmdBufferInheritanceInfo.sType                          = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
            return cmdBufferInheritanceInfo;
        }

        inline VkRenderPassBeginInfo renderPassBeginInfo()
        {
            VkRenderPassBeginInfo renderPassBeginInfo = {};
            renderPassBeginInfo.sType                 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            return renderPassBeginInfo;
        }

        inline VkRenderPassCreateInfo RenderPassCreateInfo()
        {
            VkRenderPassCreateInfo renderPassCreateInfo = {};
            renderPassCreateInfo.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            return renderPassCreateInfo;
        }

        /** @brief Initialize an image memory barrier with no image transfer ownership */
        inline VkImageMemoryBarrier ImageMemoryBarrier()
        {
            VkImageMemoryBarrier imageMemoryBarrier = {};
            imageMemoryBarrier.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.srcQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.dstQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
            return imageMemoryBarrier;
        }

        /** @brief Initialize a buffer memory barrier with no image transfer ownership */
        inline VkBufferMemoryBarrier BufferMemoryBarrier()
        {
            VkBufferMemoryBarrier bufferMemoryBarrier = {};
            bufferMemoryBarrier.sType                 = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            bufferMemoryBarrier.srcQueueFamilyIndex   = VK_QUEUE_FAMILY_IGNORED;
            bufferMemoryBarrier.dstQueueFamilyIndex   = VK_QUEUE_FAMILY_IGNORED;
            return bufferMemoryBarrier;
        }

        inline VkMemoryBarrier MemoryBarrier()
        {
            VkMemoryBarrier memoryBarrier = {};
            memoryBarrier.sType           = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
            return memoryBarrier;
        }

        inline VkImageCreateInfo ImageCreateInfo()
        {
            VkImageCreateInfo imageCreateInfo = {};
            imageCreateInfo.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            return imageCreateInfo;
        }

        inline VkSamplerCreateInfo SamplerCreateInfo()
        {
            VkSamplerCreateInfo samplerCreateInfo = {};
            samplerCreateInfo.sType               = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerCreateInfo.maxAnisotropy       = 1.0f;
            return samplerCreateInfo;
        }

        inline VkImageViewCreateInfo ImageViewCreateInfo()
        {
            VkImageViewCreateInfo imageViewCreateInfo {};
            imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            return imageViewCreateInfo;
        }

        inline VkFramebufferCreateInfo FramebufferCreateInfo()
        {
            VkFramebufferCreateInfo framebufferCreateInfo = {};
            framebufferCreateInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            return framebufferCreateInfo;
        }

        inline VkSemaphoreCreateInfo SemaphoreCreateInfo()
        {
            VkSemaphoreCreateInfo semaphoreCreateInfo = {};
            semaphoreCreateInfo.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            return semaphoreCreateInfo;
        }

        inline VkFenceCreateInfo FenceCreateInfo(VkFenceCreateFlags flags = 0)
        {
            VkFenceCreateInfo fenceCreateInfo = {};
            fenceCreateInfo.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceCreateInfo.flags             = flags;
            return fenceCreateInfo;
        }

        inline VkEventCreateInfo EventCreateInfo()
        {
            VkEventCreateInfo eventCreateInfo = {};
            eventCreateInfo.sType             = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
            return eventCreateInfo;
        }

        inline VkSubmitInfo SubmitInfo()
        {
            VkSubmitInfo submitInfo = {};
            submitInfo.sType        = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.pNext        = VK_NULL_HANDLE;
            return submitInfo;
        }

        inline VkViewport Viewport(
            float width,
            float height,
            float minDepth,
            float maxDepth)
        {
            VkViewport viewport = {};
            viewport.width      = width;
            viewport.height     = height;
            viewport.minDepth   = minDepth;
            viewport.maxDepth   = maxDepth;
            return viewport;
        }

        inline VkRect2D Rect2D(
            int32_t width,
            int32_t height,
            int32_t offsetX,
            int32_t offsetY)
        {
            VkRect2D rect2D      = {};
            rect2D.extent.width  = width;
            rect2D.extent.height = height;
            rect2D.offset.x      = offsetX;
            rect2D.offset.y      = offsetY;
            return rect2D;
        }

        inline VkBufferCreateInfo BufferCreateInfo()
        {
            VkBufferCreateInfo bufferCreateInfo = {};
            bufferCreateInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            return bufferCreateInfo;
        }

        inline VkBufferCreateInfo BufferCreateInfo(
            VkBufferUsageFlags usage,
            VkDeviceSize size)
        {
            VkBufferCreateInfo bufferCreateInfo = {};
            bufferCreateInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferCreateInfo.usage              = usage;
            bufferCreateInfo.size               = size;
            return bufferCreateInfo;
        }

        inline VkDescriptorPoolCreateInfo DescriptorPoolCreateInfo(
            uint32_t poolSizeCount,
            VkDescriptorPoolSize* pPoolSizes,
            uint32_t maxSets)
        {
            VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
            descriptorPoolInfo.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            descriptorPoolInfo.poolSizeCount              = poolSizeCount;
            descriptorPoolInfo.pPoolSizes                 = pPoolSizes;
            descriptorPoolInfo.maxSets                    = maxSets;
            return descriptorPoolInfo;
        }

        inline VkDescriptorPoolCreateInfo DescriptorPoolCreateInfo(
            const TDArray<VkDescriptorPoolSize>& poolSizes,
            uint32_t maxSets)
        {
            VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
            descriptorPoolInfo.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            descriptorPoolInfo.poolSizeCount              = static_cast<uint32_t>(poolSizes.Size());
            descriptorPoolInfo.pPoolSizes                 = poolSizes.Data();
            descriptorPoolInfo.maxSets                    = maxSets;
            return descriptorPoolInfo;
        }

        inline VkDescriptorPoolSize DescriptorPoolSize(
            VkDescriptorType type,
            uint32_t descriptorCount)
        {
            VkDescriptorPoolSize descriptorPoolSize = {};
            descriptorPoolSize.type                 = type;
            descriptorPoolSize.descriptorCount      = descriptorCount;
            return descriptorPoolSize;
        }

        inline VkDescriptorSetLayoutBinding DescriptorSetLayoutBinding(
            VkDescriptorType type,
            VkShaderStageFlags stageFlags,
            uint32_t binding,
            uint32_t descriptorCount = 1)
        {
            VkDescriptorSetLayoutBinding setLayoutBinding = {};
            setLayoutBinding.descriptorType               = type;
            setLayoutBinding.stageFlags                   = stageFlags;
            setLayoutBinding.binding                      = binding;
            setLayoutBinding.descriptorCount              = descriptorCount;
            return setLayoutBinding;
        }

        inline VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutCreateInfo(
            const VkDescriptorSetLayoutBinding* pBindings,
            uint32_t bindingCount)
        {
            VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
            descriptorSetLayoutCreateInfo.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descriptorSetLayoutCreateInfo.pBindings                       = pBindings;
            descriptorSetLayoutCreateInfo.bindingCount                    = bindingCount;
            return descriptorSetLayoutCreateInfo;
        }

        inline VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutCreateInfo(
            const TDArray<VkDescriptorSetLayoutBinding>& bindings)
        {
            VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
            descriptorSetLayoutCreateInfo.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descriptorSetLayoutCreateInfo.pBindings                       = bindings.Data();
            descriptorSetLayoutCreateInfo.bindingCount                    = static_cast<uint32_t>(bindings.Size());
            return descriptorSetLayoutCreateInfo;
        }

        inline VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo(
            const VkDescriptorSetLayout* pSetLayouts,
            uint32_t setLayoutCount = 1)
        {
            VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
            pipelineLayoutCreateInfo.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutCreateInfo.setLayoutCount             = setLayoutCount;
            pipelineLayoutCreateInfo.pSetLayouts                = pSetLayouts;
            return pipelineLayoutCreateInfo;
        }

        inline VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo(
            uint32_t setLayoutCount = 1)
        {
            VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
            pipelineLayoutCreateInfo.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutCreateInfo.setLayoutCount             = setLayoutCount;
            return pipelineLayoutCreateInfo;
        }

        inline VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo(
            VkDescriptorPool descriptorPool,
            const VkDescriptorSetLayout* pSetLayouts,
            uint32_t descriptorSetCount)
        {
            VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
            descriptorSetAllocateInfo.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            descriptorSetAllocateInfo.descriptorPool              = descriptorPool;
            descriptorSetAllocateInfo.pSetLayouts                 = pSetLayouts;
            descriptorSetAllocateInfo.descriptorSetCount          = descriptorSetCount;
            return descriptorSetAllocateInfo;
        }

        inline VkDescriptorImageInfo DescriptorImageInfo(VkSampler sampler, VkImageView imageView, VkImageLayout imageLayout)
        {
            VkDescriptorImageInfo descriptorImageInfo = {};
            descriptorImageInfo.sampler               = sampler;
            descriptorImageInfo.imageView             = imageView;
            descriptorImageInfo.imageLayout           = imageLayout;
            return descriptorImageInfo;
        }

        inline VkWriteDescriptorSet WriteDescriptorSet(
            VkDescriptorSet dstSet,
            VkDescriptorType type,
            uint32_t binding,
            VkDescriptorBufferInfo* bufferInfo,
            uint32_t descriptorCount = 1)
        {
            VkWriteDescriptorSet writeDescriptorSet = {};
            writeDescriptorSet.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSet.dstSet               = dstSet;
            writeDescriptorSet.descriptorType       = type;
            writeDescriptorSet.dstBinding           = binding;
            writeDescriptorSet.pBufferInfo          = bufferInfo;
            writeDescriptorSet.descriptorCount      = descriptorCount;
            return writeDescriptorSet;
        }

        inline VkWriteDescriptorSet WriteDescriptorSet(
            VkDescriptorSet dstSet,
            VkDescriptorType type,
            uint32_t binding,
            VkDescriptorImageInfo* imageInfo,
            uint32_t descriptorCount = 1)
        {
            VkWriteDescriptorSet writeDescriptorSet = {};
            writeDescriptorSet.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSet.dstSet               = dstSet;
            writeDescriptorSet.descriptorType       = type;
            writeDescriptorSet.dstBinding           = binding;
            writeDescriptorSet.pImageInfo           = imageInfo;
            writeDescriptorSet.descriptorCount      = descriptorCount;
            return writeDescriptorSet;
        }

        inline VkVertexInputBindingDescription VertexInputBindingDescription(
            uint32_t binding,
            uint32_t stride,
            VkVertexInputRate inputRate)
        {
            VkVertexInputBindingDescription vInputBindDescription = {};
            vInputBindDescription.binding                         = binding;
            vInputBindDescription.stride                          = stride;
            vInputBindDescription.inputRate                       = inputRate;
            return vInputBindDescription;
        }

        inline VkVertexInputAttributeDescription VertexInputAttributeDescription(
            uint32_t binding,
            uint32_t location,
            VkFormat format,
            uint32_t offset)
        {
            VkVertexInputAttributeDescription vInputAttribDescription = {};
            vInputAttribDescription.location                          = location;
            vInputAttribDescription.binding                           = binding;
            vInputAttribDescription.format                            = format;
            vInputAttribDescription.offset                            = offset;
            return vInputAttribDescription;
        }

        inline VkPipelineVertexInputStateCreateInfo PipelineVertexInputStateCreateInfo()
        {
            VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo = {};
            pipelineVertexInputStateCreateInfo.sType                                = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            return pipelineVertexInputStateCreateInfo;
        }

        inline VkPipelineInputAssemblyStateCreateInfo PipelineInputAssemblyStateCreateInfo(
            VkPrimitiveTopology topology,
            VkPipelineInputAssemblyStateCreateFlags flags,
            VkBool32 primitiveRestartEnable)
        {
            VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo = {};
            pipelineInputAssemblyStateCreateInfo.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            pipelineInputAssemblyStateCreateInfo.topology                               = topology;
            pipelineInputAssemblyStateCreateInfo.flags                                  = flags;
            pipelineInputAssemblyStateCreateInfo.primitiveRestartEnable                 = primitiveRestartEnable;
            return pipelineInputAssemblyStateCreateInfo;
        }

        inline VkPipelineRasterizationStateCreateInfo PipelineRasterizationStateCreateInfo(
            VkPolygonMode polygonMode,
            VkCullModeFlags cullMode,
            VkFrontFace frontFace,
            VkPipelineRasterizationStateCreateFlags flags = 0)
        {
            VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo = {};
            pipelineRasterizationStateCreateInfo.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            pipelineRasterizationStateCreateInfo.polygonMode                            = polygonMode;
            pipelineRasterizationStateCreateInfo.cullMode                               = cullMode;
            pipelineRasterizationStateCreateInfo.frontFace                              = frontFace;
            pipelineRasterizationStateCreateInfo.flags                                  = flags;
            pipelineRasterizationStateCreateInfo.depthClampEnable                       = VK_FALSE;
            pipelineRasterizationStateCreateInfo.lineWidth                              = 1.0f;
            return pipelineRasterizationStateCreateInfo;
        }

        inline VkPipelineColorBlendAttachmentState PipelineColourBlendAttachmentState(
            VkColorComponentFlags colorWriteMask,
            VkBool32 blendEnable)
        {
            VkPipelineColorBlendAttachmentState pipelineColourBlendAttachmentState = {};
            pipelineColourBlendAttachmentState.colorWriteMask                      = colorWriteMask;
            pipelineColourBlendAttachmentState.blendEnable                         = blendEnable;
            return pipelineColourBlendAttachmentState;
        }

        inline VkPipelineColorBlendStateCreateInfo PipelineColourBlendStateCreateInfo(
            uint32_t attachmentCount,
            const VkPipelineColorBlendAttachmentState* pAttachments)
        {
            VkPipelineColorBlendStateCreateInfo pipelineColourBlendStateCreateInfo = {};
            pipelineColourBlendStateCreateInfo.sType                               = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            pipelineColourBlendStateCreateInfo.attachmentCount                     = attachmentCount;
            pipelineColourBlendStateCreateInfo.pAttachments                        = pAttachments;
            return pipelineColourBlendStateCreateInfo;
        }

        inline VkPipelineDepthStencilStateCreateInfo PipelineDepthStencilStateCreateInfo(
            VkBool32 depthTestEnable,
            VkBool32 depthWriteEnable,
            VkCompareOp depthCompareOp)
        {
            VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo = {};
            pipelineDepthStencilStateCreateInfo.sType                                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            pipelineDepthStencilStateCreateInfo.depthTestEnable                       = depthTestEnable;
            pipelineDepthStencilStateCreateInfo.depthWriteEnable                      = depthWriteEnable;
            pipelineDepthStencilStateCreateInfo.depthCompareOp                        = depthCompareOp;
            pipelineDepthStencilStateCreateInfo.front                                 = pipelineDepthStencilStateCreateInfo.back;
            pipelineDepthStencilStateCreateInfo.back.compareOp                        = VK_COMPARE_OP_ALWAYS;
            return pipelineDepthStencilStateCreateInfo;
        }

        inline VkPipelineViewportStateCreateInfo PipelineViewportStateCreateInfo(
            uint32_t viewportCount,
            uint32_t scissorCount,
            VkPipelineViewportStateCreateFlags flags = 0)
        {
            VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo = {};
            pipelineViewportStateCreateInfo.sType                             = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            pipelineViewportStateCreateInfo.viewportCount                     = viewportCount;
            pipelineViewportStateCreateInfo.scissorCount                      = scissorCount;
            pipelineViewportStateCreateInfo.flags                             = flags;
            return pipelineViewportStateCreateInfo;
        }

        inline VkPipelineMultisampleStateCreateInfo PipelineMultisampleStateCreateInfo(
            VkSampleCountFlagBits rasterizationSamples,
            VkPipelineMultisampleStateCreateFlags flags = 0)
        {
            VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo = {};
            pipelineMultisampleStateCreateInfo.sType                                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            pipelineMultisampleStateCreateInfo.rasterizationSamples                 = rasterizationSamples;
            pipelineMultisampleStateCreateInfo.flags                                = flags;
            return pipelineMultisampleStateCreateInfo;
        }

        inline VkPipelineDynamicStateCreateInfo PipelineDynamicStateCreateInfo(
            const VkDynamicState* pDynamicStates,
            uint32_t dynamicStateCount,
            VkPipelineDynamicStateCreateFlags flags = 0)
        {
            VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo = {};
            pipelineDynamicStateCreateInfo.sType                            = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            pipelineDynamicStateCreateInfo.pDynamicStates                   = pDynamicStates;
            pipelineDynamicStateCreateInfo.dynamicStateCount                = dynamicStateCount;
            pipelineDynamicStateCreateInfo.flags                            = flags;
            return pipelineDynamicStateCreateInfo;
        }

        inline VkPipelineDynamicStateCreateInfo PipelineDynamicStateCreateInfo(
            const TDArray<VkDynamicState>& pDynamicStates,
            VkPipelineDynamicStateCreateFlags flags = 0)
        {
            VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo = {};
            pipelineDynamicStateCreateInfo.sType                            = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            pipelineDynamicStateCreateInfo.pDynamicStates                   = pDynamicStates.Data();
            pipelineDynamicStateCreateInfo.dynamicStateCount                = static_cast<uint32_t>(pDynamicStates.Size());
            pipelineDynamicStateCreateInfo.flags                            = flags;
            return pipelineDynamicStateCreateInfo;
        }

        inline VkPipelineTessellationStateCreateInfo PipelineTessellationStateCreateInfo(uint32_t patchControlPoints)
        {
            VkPipelineTessellationStateCreateInfo pipelineTessellationStateCreateInfo = {};
            pipelineTessellationStateCreateInfo.sType                                 = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
            pipelineTessellationStateCreateInfo.patchControlPoints                    = patchControlPoints;
            return pipelineTessellationStateCreateInfo;
        }

        inline VkGraphicsPipelineCreateInfo PipelineDesc(
            VkPipelineLayout layout,
            VkRenderPass renderPass,
            VkPipelineCreateFlags flags = 0)
        {
            VkGraphicsPipelineCreateInfo pipelineDesc = {};
            pipelineDesc.sType                        = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            pipelineDesc.layout                       = layout;
            pipelineDesc.renderPass                   = renderPass;
            pipelineDesc.flags                        = flags;
            pipelineDesc.basePipelineIndex            = -1;
            pipelineDesc.basePipelineHandle           = VK_NULL_HANDLE;
            return pipelineDesc;
        }

        inline VkGraphicsPipelineCreateInfo PipelineDesc()
        {
            VkGraphicsPipelineCreateInfo pipelineDesc = {};
            pipelineDesc.sType                        = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            pipelineDesc.basePipelineIndex            = -1;
            pipelineDesc.basePipelineHandle           = VK_NULL_HANDLE;
            return pipelineDesc;
        }

        inline VkComputePipelineCreateInfo ComputePipelineCreateInfo(
            VkPipelineLayout layout,
            VkPipelineCreateFlags flags = 0)
        {
            VkComputePipelineCreateInfo computePipelineCreateInfo = {};
            computePipelineCreateInfo.sType                       = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
            computePipelineCreateInfo.layout                      = layout;
            computePipelineCreateInfo.flags                       = flags;
            return computePipelineCreateInfo;
        }

        inline VkPushConstantRange PushConstantRange(
            VkShaderStageFlags stageFlags,
            uint32_t size,
            uint32_t offset)
        {
            VkPushConstantRange pushConstantRange = {};
            pushConstantRange.stageFlags          = stageFlags;
            pushConstantRange.offset              = offset;
            pushConstantRange.size                = size;
            return pushConstantRange;
        }

        inline VkBindSparseInfo BindSparseInfo()
        {
            VkBindSparseInfo bindSparseInfo = {};
            bindSparseInfo.sType            = VK_STRUCTURE_TYPE_BIND_SPARSE_INFO;
            return bindSparseInfo;
        }

        /** @brief Initialize a map entry for a shader specialization constant */
        inline VkSpecializationMapEntry SpecializationMapEntry(uint32_t constantID, uint32_t offset, size_t size)
        {
            VkSpecializationMapEntry specializationMapEntry = {};
            specializationMapEntry.constantID               = constantID;
            specializationMapEntry.offset                   = offset;
            specializationMapEntry.size                     = size;
            return specializationMapEntry;
        }

        /** @brief Initialize a specialization constant info structure to pass to a shader stage */
        inline VkSpecializationInfo SpecializationInfo(uint32_t mapEntryCount, const VkSpecializationMapEntry* mapEntries, size_t dataSize, const void* data)
        {
            VkSpecializationInfo specializationInfo = {};
            specializationInfo.mapEntryCount        = mapEntryCount;
            specializationInfo.pMapEntries          = mapEntries;
            specializationInfo.dataSize             = dataSize;
            specializationInfo.pData                = data;
            return specializationInfo;
        }
    }
}
