#include "Precompiled.h"
#include "VKBuffer.h"
#include "VKDevice.h"
#include "VKRenderer.h"
#include "VKUtilities.h"

namespace Lumos
{
    namespace Graphics
    {
        VKBuffer::VKBuffer(VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperyFlags, uint32_t size, const void* data)
            : m_Size(size)
        {
            Init(usage, memoryProperyFlags, size, data);
        }

        VKBuffer::VKBuffer()
            : m_Size(0)
        {
        }

        VKBuffer::~VKBuffer()
        {
            LUMOS_PROFILE_FUNCTION();
            if(m_Buffer)
            {
                VKContext::DeletionQueue& deletionQueue = VKRenderer::GetCurrentDeletionQueue();

                auto buffer = m_Buffer;

#ifdef USE_VMA_ALLOCATOR
                auto alloc = m_Allocation;
                deletionQueue.PushFunction([buffer, alloc]
                    { vmaDestroyBuffer(VKDevice::Get().GetAllocator(), buffer, alloc); });
#else
                auto memory = m_Memory;
                deletionQueue.PushFunction([buffer, memory]
                    {
                        vkDestroyBuffer(VKDevice::Device(), buffer, nullptr);
                        vkFreeMemory(VKDevice::Device(), memory, nullptr); });
#endif
            }
        }

        void VKBuffer::Destroy(bool deletionQueue)
        {
            LUMOS_PROFILE_FUNCTION();
            if(m_Buffer)
            {
                VKContext::DeletionQueue& currentDeletionQueue = VKRenderer::GetCurrentDeletionQueue();

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
                            vkDestroyBuffer(VKDevice::Device(), buffer, nullptr);
                            vkFreeMemory(VKDevice::Device(), memory, nullptr); });
#endif
                }
                else
                {
#ifdef USE_VMA_ALLOCATOR
                    vmaDestroyBuffer(VKDevice::Get().GetAllocator(), buffer, m_Allocation);
#else
                    vkDestroyBuffer(VKDevice::Device(), buffer, nullptr);
                    vkFreeMemory(VKDevice::Device(), memory, nullptr);
#endif
                }
            }

            m_Buffer = VK_NULL_HANDLE;
        }

        void VKBuffer::Init(VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperyFlags, uint32_t size, const void* data)
        {
            LUMOS_PROFILE_FUNCTION();

            m_UsageFlags = usage;
            m_MemoryProperyFlags = memoryProperyFlags;
            m_Size = size;

            VkBufferCreateInfo bufferInfo = {};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = size;
            bufferInfo.usage = usage;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            bool isMappable = (memoryProperyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0;

#ifdef USE_VMA_ALLOCATOR
            VmaAllocationCreateInfo vmaAllocInfo = {};
            vmaAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
            vmaAllocInfo.flags = isMappable ? VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT : 0;
            vmaAllocInfo.preferredFlags = memoryProperyFlags;

            vmaCreateBuffer(VKDevice::Get().GetAllocator(), &bufferInfo, &vmaAllocInfo, &m_Buffer, &m_Allocation, nullptr);
#else
            VK_CHECK_RESULT(vkCreateBuffer(VKDevice::Device(), &bufferInfo, nullptr, &m_Buffer));

            VkMemoryRequirements memRequirements;
            vkGetBufferMemoryRequirements(VKDevice::Device(), m_Buffer, &memRequirements);

            VkMemoryAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = VKUtilities::FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

            VK_CHECK_RESULT(vkAllocateMemory(VKDevice::Device(), &allocInfo, nullptr, &m_Memory));

            vkBindBufferMemory(VKDevice::Device(), m_Buffer, m_Memory, 0);
#endif

            if(data != nullptr)
                SetData(size, data);
        }

        void VKBuffer::SetData(uint32_t size, const void* data)
        {
            LUMOS_PROFILE_FUNCTION();
            Map(size, 0);
            memcpy(m_Mapped, data, size);
            UnMap();
        }

        void VKBuffer::Resize(uint32_t size, const void* data)
        {
            auto usage = m_UsageFlags;

            if(m_Buffer)
            {
#ifdef USE_VMA_ALLOCATOR
                vmaDestroyBuffer(VKDevice::Get().GetAllocator(), m_Buffer, m_Allocation);
#else
                vkDestroyBuffer(VKDevice::Device(), m_Buffer, nullptr);

                if(m_Memory)
                {
                    vkFreeMemory(VKDevice::Device(), m_Memory, nullptr);
                }
#endif
            }

            Init(usage, m_MemoryProperyFlags, size, data);
        }

        void VKBuffer::Map(VkDeviceSize size, VkDeviceSize offset)
        {
            LUMOS_PROFILE_FUNCTION();
#ifdef USE_VMA_ALLOCATOR
            VkResult res = static_cast<VkResult>(vmaMapMemory(VKDevice::Get().GetAllocator(), m_Allocation, (void**)&m_Mapped));
#else
            VkResult res = vkMapMemory(VKDevice::Device(), m_Memory, offset, size, 0, &m_Mapped);
#endif
            if(res != VK_SUCCESS)
                LUMOS_LOG_CRITICAL("[VULKAN] Failed to map buffer");
        }

        void VKBuffer::UnMap()
        {
            LUMOS_PROFILE_FUNCTION();
            if(m_Mapped)
            {
#ifdef USE_VMA_ALLOCATOR
                vmaUnmapMemory(VKDevice::Get().GetAllocator(), m_Allocation);
#else
                vkUnmapMemory(VKDevice::Device(), m_Memory);
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
            mappedRange.memory = m_Memory;
            mappedRange.offset = offset;
            mappedRange.size = size;
            vkFlushMappedMemoryRanges(VKDevice::Device(), 1, &mappedRange);
#endif
        }

        void VKBuffer::Invalidate(VkDeviceSize size, VkDeviceSize offset)
        {
            LUMOS_PROFILE_FUNCTION();
#ifdef USE_VMA_ALLOCATOR
            vmaInvalidateAllocation(VKDevice::Get().GetAllocator(), m_Allocation, offset, size);
#else
            VkMappedMemoryRange mappedRange = {};
            mappedRange.memory = m_Memory;
            mappedRange.offset = offset;
            mappedRange.size = size;
            vkInvalidateMappedMemoryRanges(VKDevice::Device(), 1, &mappedRange);
#endif
        }
    }
}
