#pragma once

#include "VK.h"

#ifdef USE_VMA_ALLOCATOR
#include <vulkan/vk_mem_alloc.h>
#endif

namespace Lumos
{
    namespace Graphics
    {
        class VKBuffer
        {
        public:
            VKBuffer(VkBufferUsageFlags usage, uint32_t size, const void* data);
            VKBuffer();
            virtual ~VKBuffer();

            void Init(VkBufferUsageFlags usage, uint32_t size, const void* data);
            void Resize(uint32_t size, const void* data);
            void SetData(uint32_t size, const void* data);
            const VkBuffer& GetBuffer() const { return m_Buffer; }

            const VkDescriptorBufferInfo& GetBufferInfo() const { return m_DesciptorBufferInfo; };

            void Map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
            void UnMap();
            void Flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
            void Invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
            void SetUsage(VkBufferUsageFlags flags) { m_UsageFlags = flags; }

        protected:
            VkBuffer m_Buffer {};
            VkDeviceMemory m_Memory {};
            VkDescriptorBufferInfo m_DesciptorBufferInfo {};
            VkDeviceSize m_Size = 0;
            VkDeviceSize m_Alignment = 0;
            VkBufferUsageFlags m_UsageFlags;
            void* m_Mapped = nullptr;

#ifdef USE_VMA_ALLOCATOR
            VmaAllocation m_Allocation {};
            VmaAllocation m_MappedAllocation {};
#endif
        };
    }
}
