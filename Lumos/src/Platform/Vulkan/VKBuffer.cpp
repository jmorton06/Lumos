#include "LM.h"
#include "VKBuffer.h"
#include "VKDevice.h"
#include "VKRenderer.h"

namespace lumos
{
	namespace graphics
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
				vkDestroyBuffer(VKDevice::Instance()->GetDevice(), m_Buffer, nullptr);
			}

			if (m_Memory)
			{
				vkFreeMemory(VKDevice::Instance()->GetDevice(), m_Memory, nullptr);
			}
		}

		void VKBuffer::Init(vk::BufferUsageFlags usage, uint32_t size, const void* data)
		{
			vk::BufferCreateInfo bufferInfo = {};
			bufferInfo.size = size;
			bufferInfo.usage = usage;
			bufferInfo.sharingMode = vk::SharingMode::eExclusive;

			m_Buffer = VKDevice::Instance()->GetDevice().createBuffer(bufferInfo);

			vk::MemoryRequirements memRequirements = VKDevice::Instance()->GetDevice().getBufferMemoryRequirements(m_Buffer);

			vk::MemoryAllocateInfo allocInfo = {};
			allocInfo.allocationSize = memRequirements.size;
			allocInfo.memoryTypeIndex = VKTools::FindMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

			vk::Result result = VKDevice::Instance()->GetDevice().allocateMemory(&allocInfo, nullptr, &m_Memory);
			if (result != vk::Result::eSuccess)
			{
				throw std::runtime_error("failed to allocate buffer memory!");
			}

			VKDevice::Instance()->GetDevice().bindBufferMemory(m_Buffer, m_Memory, 0);

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
			VKDevice::Instance()->GetDevice().mapMemory(m_Memory, offset, size, vk::MemoryMapFlagBits(), &m_Mapped);
		}

		void VKBuffer::UnMap()
		{
			if (m_Mapped)
			{
				VKDevice::Instance()->GetDevice().unmapMemory(m_Memory);
				m_Mapped = nullptr;
			}
		}

		void VKBuffer::Flush(VkDeviceSize size, VkDeviceSize offset)
		{
			vk::MappedMemoryRange mappedRange = {};
			mappedRange.memory = m_Memory;
			mappedRange.offset = offset;
			mappedRange.size = size;
			VKDevice::Instance()->GetDevice().flushMappedMemoryRanges(1, &mappedRange);
		}
		
		void VKBuffer::Invalidate(VkDeviceSize size, VkDeviceSize offset)
		{
			vk::MappedMemoryRange mappedRange = {};
			mappedRange.memory = m_Memory;
			mappedRange.offset = offset;
			mappedRange.size = size;
			VKDevice::Instance()->GetDevice().invalidateMappedMemoryRanges(1, &mappedRange);
		}
	}
}
