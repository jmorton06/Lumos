#include "LM.h"
#include "VKBuffer.h"
#include "VKDevice.h"
#include "VKRenderer.h"

namespace Lumos
{
	namespace Graphics
	{
		VKBuffer::VKBuffer(vk::BufferUsageFlags usage, uint32_t size, const void* data) : m_Size(size)
		{
			Init(usage, size, data);
		}

		VKBuffer::VKBuffer() : m_Size(0)
		{
		}

		VKBuffer::~VKBuffer()
		{
			if (m_Buffer)
			{
#ifdef USE_VMA_ALLOCATOR
                vmaDestroyBuffer(VKDevice::Instance()->GetAllocator(), m_Buffer, m_Allocation);
#else
				VKDevice::Instance()->GetDevice().destroyBuffer(m_Buffer);

				if (m_Memory)
				{
					VKDevice::Instance()->GetDevice().freeMemory(m_Memory);
				}
#endif
			}
		}

		void VKBuffer::Init(vk::BufferUsageFlags usage, uint32_t size, const void* data)
		{
			vk::BufferCreateInfo bufferInfo = {};
			bufferInfo.size = size;
			bufferInfo.usage = usage;
			bufferInfo.sharingMode = vk::SharingMode::eExclusive;

#ifdef USE_VMA_ALLOCATOR
            VmaAllocationCreateInfo vmaAllocInfo = {};
            vmaAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
            vmaCreateBuffer(VKDevice::Instance()->GetAllocator(), (VkBufferCreateInfo*)&bufferInfo, &vmaAllocInfo, (VkBuffer*)&m_Buffer, &m_Allocation, nullptr);
#else
			m_Buffer = VKDevice::Instance()->GetDevice().createBuffer(bufferInfo);

			vk::MemoryRequirements memRequirements = VKDevice::Instance()->GetDevice().getBufferMemoryRequirements(m_Buffer);

			vk::MemoryAllocateInfo allocInfo = {};
			allocInfo.allocationSize = memRequirements.size;
			allocInfo.memoryTypeIndex = VKTools::FindMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

			vk::Result result = VKDevice::Instance()->GetDevice().allocateMemory(&allocInfo, nullptr, &m_Memory);

			if (result != vk::Result::eSuccess)
			{
				LUMOS_LOG_CRITICAL("[VULKAN] Failed to allocate buffer memory!");
			}

			VKDevice::Instance()->GetDevice().bindBufferMemory(m_Buffer, m_Memory, 0);
#endif

			if(data != nullptr)
				SetData(size, data);
		}

		void VKBuffer::SetData(uint32_t size, const void* data)
		{
			Map(size, 0);
			memcpy(m_Mapped, data, size);
			UnMap();
		}
		
		void VKBuffer::Map(VkDeviceSize size, VkDeviceSize offset)
		{
#ifdef USE_VMA_ALLOCATOR
			vk::Result res = static_cast<vk::Result>(vmaMapMemory(VKDevice::Instance()->GetAllocator(), m_Allocation, &m_Mapped));
#else
			vk::Result res = VKDevice::Instance()->GetDevice().mapMemory(m_Memory, offset, size, vk::MemoryMapFlagBits(), &m_Mapped);
#endif
			if (res != vk::Result::eSuccess)
				LUMOS_LOG_CRITICAL("[VULKAN] Failed to map buffer");
		}

		void VKBuffer::UnMap()
		{
			if (m_Mapped)
			{
#ifdef USE_VMA_ALLOCATOR
				vmaUnmapMemory(VKDevice::Instance()->GetAllocator(), m_Allocation);
#else
				VKDevice::Instance()->GetDevice().unmapMemory(m_Memory);
#endif
				m_Memory = nullptr;
			}
		}

		void VKBuffer::Flush(VkDeviceSize size, VkDeviceSize offset)
		{
#ifdef USE_VMA_ALLOCATOR
			vmaFlushAllocation(VKDevice::Instance()->GetAllocator(), m_Allocation, offset, size);
#else
			vk::MappedMemoryRange mappedRange = {};
			mappedRange.memory = m_Memory;
			mappedRange.offset = offset;
			mappedRange.size = size;
			VKDevice::Instance()->GetDevice().flushMappedMemoryRanges(1, &mappedRange);
#endif
		}
		
		void VKBuffer::Invalidate(VkDeviceSize size, VkDeviceSize offset)
		{
#ifdef USE_VMA_ALLOCATOR
			vmaInvalidateAllocation(VKDevice::Instance()->GetAllocator(), m_Allocation, offset, size);
#else
			vk::MappedMemoryRange mappedRange = {};
			mappedRange.memory = m_Memory;
			mappedRange.offset = offset;
			mappedRange.size = size;
			VKDevice::Instance()->GetDevice().invalidateMappedMemoryRanges(1, &mappedRange);
#endif
		}
	}
}
