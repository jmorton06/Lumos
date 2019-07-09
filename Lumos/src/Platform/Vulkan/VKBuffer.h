#pragma once

#include "VK.h"

#ifdef USE_VMA_ALLOCATOR
#include "vk_mem_alloc.h"
#endif

namespace Lumos
{
	namespace Graphics
	{
		class VKBuffer
		{
		public:
			VKBuffer(vk::BufferUsageFlags usage, uint32_t size, const void* data);
			VKBuffer();
			virtual ~VKBuffer();

			void Init(vk::BufferUsageFlags usage, uint32_t size, const void* data);

			void SetData(uint32_t size, const void* data);
			vk::Buffer GetBuffer() const { return m_Buffer; }

			vk::DescriptorBufferInfo GetBufferInfo() const { return m_DesciptorBufferInfo; };

			void Map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
			void UnMap();
			void Flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
			void Invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

		protected:
			vk::Buffer m_Buffer{};
			vk::DeviceMemory m_Memory{};
			vk::DescriptorBufferInfo m_DesciptorBufferInfo;
			VkDeviceSize m_Size = 0;
			VkDeviceSize m_Alignment = 0;
			void* m_Mapped = nullptr;
            
#ifdef USE_VMA_ALLOCATOR
            VmaAllocation m_Allocation;
#endif
		};
	}
}
