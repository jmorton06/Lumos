#pragma once

#include "VK.h"

namespace Lumos
{
    namespace Graphics
    {
        class VKBuffer
        {
        public:
            VKBuffer(VkBufferUsageFlags usage, VkMemoryPropertyFlags MemoryPropertyFlags, uint32_t size, const void* data);
            VKBuffer();
            virtual ~VKBuffer();

            void Init(VkBufferUsageFlags usage, VkMemoryPropertyFlags MemoryPropertyFlags, uint32_t size, const void* data);
            void Resize(uint32_t size, const void* data);
            void SetData(uint32_t size, const void* data, bool addBarrier = false);
            const VkBuffer& GetBuffer() const { return m_Buffer; }

            const VkDescriptorBufferInfo& GetBufferInfo() const { return m_DesciptorBufferInfo; };

            void Map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
            void UnMap();
            void Flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
            void Invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
            void SetUsage(VkBufferUsageFlags flags) { m_UsageFlags = flags; }
            void SetMemoryPropertyFlags(VkBufferUsageFlags flags) { m_MemoryPropertyFlags = flags; }
            void Destroy(bool deletionQueue = false);
            void SetDeleteWithoutQueue(bool value) { m_DeleteWithoutQueue = value; }

        protected:
            VkBuffer m_Buffer {};
            VkDeviceMemory m_Memory {};
            VkDescriptorBufferInfo m_DesciptorBufferInfo {};
            VkDeviceSize m_Size      = 0;
            VkDeviceSize m_Alignment = 0;
            VkBufferUsageFlags m_UsageFlags;
            VkMemoryPropertyFlags m_MemoryPropertyFlags;
            void* m_Mapped            = nullptr;
            bool m_DeleteWithoutQueue = false;
            bool m_GPUOnlyMemory      = false;

#ifdef USE_VMA_ALLOCATOR
            VmaAllocation m_Allocation {};
            VmaAllocation m_MappedAllocation {};
            VmaAllocationInfo m_AllocationInfo {};
#endif
        };
    }
}
