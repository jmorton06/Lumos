#include "Precompiled.h"
#include "VKBuffer.h"
#include "VKDevice.h"
#include "VKRenderer.h"
#include "VKUtilities.h"

#ifdef LUMOS_PLATFORM_WINDOWS
#define USE_SMALL_VMA_POOL 0
#else
#define USE_SMALL_VMA_POOL 0
#endif

namespace Lumos
{
    namespace Graphics
    {
        VKBuffer::VKBuffer(VkBufferUsageFlags usage, VkMemoryPropertyFlags MemoryPropertyFlags, uint32_t size, const void* data)
            : m_Size(size)
        {
            Init(usage, MemoryPropertyFlags, size, data);
        }

        VKBuffer::VKBuffer()
            : m_Size(0)
            , m_MemoryPropertyFlags(0)
            , m_UsageFlags(0)
        {
        }

        VKBuffer::~VKBuffer()
        {
            LUMOS_PROFILE_FUNCTION();
            Destroy(!m_DeleteWithoutQueue);
        }

        void VKBuffer::Destroy(bool deletionQueue)
        {
            LUMOS_PROFILE_FUNCTION();
            if(m_Buffer)
            {
                DeletionQueue& currentDeletionQueue = VKRenderer::GetCurrentDeletionQueue();

                auto buffer = m_Buffer;

                if(deletionQueue)
                {
#ifdef USE_VMA_ALLOCATOR
                    auto alloc = m_Allocation;
                    currentDeletionQueue.PushFunction([buffer, alloc]
                                                      { vmaDestroyBuffer(VKDevice::Get().GetAllocator(), buffer, alloc); });
#else
                    auto memory = m_Memory;
                    currentDeletionQueue.PushFunction([buffer, memory]
                                                      {
                                                          vkDestroyBuffer(VKDevice::Get().GetDevice(), buffer, nullptr);
                                                          vkFreeMemory(VKDevice::Get().GetDevice(), memory, nullptr); });
#endif
                }
                else
                {
#ifdef USE_VMA_ALLOCATOR
                    vmaDestroyBuffer(VKDevice::Get().GetAllocator(), buffer, m_Allocation);
#else
                    vkDestroyBuffer(VKDevice::Get().GetDevice(), buffer, nullptr);
                    vkFreeMemory(VKDevice::Get().GetDevice(), m_Memory, nullptr);
#endif
                }
            }

            m_Buffer = VK_NULL_HANDLE;
        }

        void VKBuffer::Init(VkBufferUsageFlags usage, VkMemoryPropertyFlags MemoryPropertyFlags, uint32_t size, const void* data)
        {
            LUMOS_PROFILE_FUNCTION();

            m_UsageFlags          = usage;
            m_MemoryPropertyFlags = MemoryPropertyFlags;
            m_Size                = size;

            VkBufferCreateInfo bufferInfo = {};
            bufferInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size               = size;
            bufferInfo.usage              = usage;
            bufferInfo.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;

            bool isMappable = (MemoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0;

            m_GPUOnlyMemory = !isMappable;

#ifdef USE_VMA_ALLOCATOR

            if(!data)
            {
                VmaAllocationCreateInfo vmaAllocInfo = {};
                vmaAllocInfo.usage                   = VMA_MEMORY_USAGE_AUTO;
                vmaAllocInfo.flags                   = isMappable ? VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT : 0;
                vmaAllocInfo.preferredFlags          = MemoryPropertyFlags;

#if USE_SMALL_VMA_POOL
                if(bufferInfo.size <= SMALL_ALLOCATION_MAX_SIZE)
                {
                    uint32_t mem_type_index = 0;
                    vmaFindMemoryTypeIndexForBufferInfo(VKDevice::Get().GetAllocator(), &bufferInfo, &vmaAllocInfo, &mem_type_index);
                    vmaAllocInfo.pool = VKDevice::Get().GetOrCreateSmallAllocPool(mem_type_index);
                }
#endif
                VK_CHECK_RESULT(vmaCreateBuffer(VKDevice::Get().GetAllocator(), &bufferInfo, &vmaAllocInfo, &m_Buffer, &m_Allocation, nullptr));
                return;
            }

#define USE_STAGING 1
#if USE_STAGING
            VmaAllocationCreateInfo vmaAllocInfo = {};
            vmaAllocInfo.usage                   = VMA_MEMORY_USAGE_CPU_TO_GPU;
            vmaAllocInfo.preferredFlags          = MemoryPropertyFlags;
            vmaAllocInfo.flags                   = 0;

            VkBufferCreateInfo bufferCreateInfo {};
            bufferCreateInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferCreateInfo.size        = size;
            bufferCreateInfo.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            VkBuffer stagingBuffer;
            VmaAllocation stagingAlloc;

#if USE_SMALL_VMA_POOL
            if(bufferInfo.size <= SMALL_ALLOCATION_MAX_SIZE)
            {
                uint32_t mem_type_index = 0;
                vmaFindMemoryTypeIndexForBufferInfo(VKDevice::Get().GetAllocator(), &bufferCreateInfo, &vmaAllocInfo, &mem_type_index);
                vmaAllocInfo.pool = VKDevice::Get().GetOrCreateSmallAllocPool(mem_type_index);
            }
#endif

            VK_CHECK_RESULT(vmaCreateBuffer(VKDevice::Get().GetAllocator(), &bufferCreateInfo, &vmaAllocInfo, &stagingBuffer, &stagingAlloc, nullptr));

            // Copy data to staging buffer
            uint8_t* destData;
            {
                VkResult res = static_cast<VkResult>(vmaMapMemory(VKDevice::Get().GetAllocator(), stagingAlloc, (void**)&destData));
                if(res != VK_SUCCESS)
                    LFATAL("[VULKAN] Failed to map buffer");

                memcpy(destData, data, size);
                vmaUnmapMemory(VKDevice::Get().GetAllocator(), stagingAlloc);
            }

            vmaAllocInfo.flags = isMappable ? VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT : 0;
            vmaAllocInfo.usage = isMappable ? VMA_MEMORY_USAGE_AUTO : VMA_MEMORY_USAGE_GPU_ONLY;
            bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

#if USE_SMALL_VMA_POOL
            if(bufferInfo.size <= SMALL_ALLOCATION_MAX_SIZE)
            {
                uint32_t mem_type_index = 0;
                vmaFindMemoryTypeIndexForBufferInfo(VKDevice::Get().GetAllocator(), &bufferInfo, &vmaAllocInfo, &mem_type_index);
                vmaAllocInfo.pool = VKDevice::Get().GetOrCreateSmallAllocPool(mem_type_index);
            }
#endif

            VK_CHECK_RESULT(vmaCreateBuffer(VKDevice::Get().GetAllocator(), &bufferInfo, &vmaAllocInfo, &m_Buffer, &m_Allocation, &m_AllocationInfo));

            VkCommandBuffer commandBuffer = VKUtilities::BeginSingleTimeCommands();

            VkBufferCopy copyRegion = {};
            copyRegion.size         = size;
            vkCmdCopyBuffer(
                commandBuffer,
                stagingBuffer,
                m_Buffer,
                1,
                &copyRegion);

            VKUtilities::EndSingleTimeCommands(commandBuffer);
            vmaDestroyBuffer(VKDevice::Get().GetAllocator(), stagingBuffer, stagingAlloc);
#else

            VmaAllocationCreateInfo vmaAllocInfo = {};
            vmaAllocInfo.usage                   = VMA_MEMORY_USAGE_AUTO;
            vmaAllocInfo.flags                   = isMappable ? VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT : 0;
            vmaAllocInfo.preferredFlags          = MemoryPropertyFlags;

            VK_CHECK_RESULT(vmaCreateBuffer(VKDevice::Get().GetAllocator(), &bufferInfo, &vmaAllocInfo, &m_Buffer, &m_Allocation, nullptr));

            if(data != nullptr)
                SetData(size, data);
#endif
#else
            if(size == 0)
            {
                m_Buffer = VK_NULL_HANDLE;
                m_Memory = VK_NULL_HANDLE;
                return;
            }

            VK_CHECK_RESULT(vkCreateBuffer(VKDevice::Get().GetDevice(), &bufferInfo, nullptr, &m_Buffer));

            VkMemoryRequirements memRequirements;
            vkGetBufferMemoryRequirements(VKDevice::Get().GetDevice(), m_Buffer, &memRequirements);

            VkMemoryAllocateInfo allocInfo = {};
            allocInfo.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize       = memRequirements.size;
            allocInfo.memoryTypeIndex      = VKUtilities::FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

            VK_CHECK_RESULT(vkAllocateMemory(VKDevice::Get().GetDevice(), &allocInfo, nullptr, &m_Memory));

            VK_CHECK_RESULT(vkBindBufferMemory(VKDevice::Get().GetDevice(), m_Buffer, m_Memory, 0));

            if(data != nullptr)
                SetData(size, data);
#endif
        }

        void VKBuffer::SetData(uint32_t size, const void* data, bool addBarrier)
        {
            LUMOS_PROFILE_FUNCTION();
            Map(size, 0);
            memcpy(m_Mapped, data, size);
            UnMap();

            if(addBarrier)
            {
                VkBufferMemoryBarrier bufferBarrier = {};
                bufferBarrier.sType                 = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
                bufferBarrier.srcAccessMask         = VK_ACCESS_TRANSFER_WRITE_BIT;
                bufferBarrier.dstAccessMask         = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
                bufferBarrier.srcQueueFamilyIndex   = VK_QUEUE_FAMILY_IGNORED;
                bufferBarrier.dstQueueFamilyIndex   = VK_QUEUE_FAMILY_IGNORED;
                bufferBarrier.buffer                = m_Buffer;
                bufferBarrier.offset                = 0;
                bufferBarrier.size                  = VK_WHOLE_SIZE;

                vkCmdPipelineBarrier(((VKCommandBuffer*)Renderer::GetMainSwapChain()->GetCurrentCommandBuffer())->GetHandle(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &bufferBarrier, 0, nullptr);
            }
        }

        void VKBuffer::Resize(uint32_t size, const void* data)
        {
            auto usage = m_UsageFlags;

            Destroy(!m_DeleteWithoutQueue);
            Init(usage, m_MemoryPropertyFlags, size, data);
        }

        void VKBuffer::Map(VkDeviceSize size, VkDeviceSize offset)
        {
            LUMOS_PROFILE_FUNCTION();
#ifdef USE_VMA_ALLOCATOR
            VK_CHECK_RESULT(vmaMapMemory(VKDevice::Get().GetAllocator(), m_Allocation, &m_Mapped));
            m_Mapped = (uint8_t*)m_Mapped + offset;
#else
            VK_CHECK_RESULT(vkMapMemory(VKDevice::Get().GetDevice(), m_Memory, offset, size, 0, &m_Mapped));
#endif
        }

        void VKBuffer::UnMap()
        {
            LUMOS_PROFILE_FUNCTION();
            if(m_Mapped)
            {
#ifdef USE_VMA_ALLOCATOR
                vmaUnmapMemory(VKDevice::Get().GetAllocator(), m_Allocation);
#else
                vkUnmapMemory(VKDevice::Get().GetDevice(), m_Memory);
#endif
                m_Mapped = nullptr;
            }
        }

        void VKBuffer::Flush(VkDeviceSize size, VkDeviceSize offset)
        {
            LUMOS_PROFILE_FUNCTION();
#ifdef USE_VMA_ALLOCATOR
            vmaFlushAllocation(VKDevice::Get().GetAllocator(), m_Allocation, offset, size);
#else
            VkMappedMemoryRange mappedRange = {};
            mappedRange.memory              = m_Memory;
            mappedRange.offset              = offset;
            mappedRange.size                = size;
            vkFlushMappedMemoryRanges(VKDevice::Get().GetDevice(), 1, &mappedRange);
#endif
        }

        void VKBuffer::Invalidate(VkDeviceSize size, VkDeviceSize offset)
        {
            LUMOS_PROFILE_FUNCTION();
#ifdef USE_VMA_ALLOCATOR
            vmaInvalidateAllocation(VKDevice::Get().GetAllocator(), m_Allocation, offset, size);
#else
            VkMappedMemoryRange mappedRange = {};
            mappedRange.memory              = m_Memory;
            mappedRange.offset              = offset;
            mappedRange.size                = size;
            vkInvalidateMappedMemoryRanges(VKDevice::Get().GetDevice(), 1, &mappedRange);
#endif
        }
    }
}
