#include "LM.h"
#include "VKUniformBuffer.h"
#include "VKDevice.h"
#include "VKTools.h"

namespace lumos
{
	namespace graphics
	{
		VKUniformBuffer::VKUniformBuffer(uint32_t size, const void* data)
		{
			VKTools::CreateBuffer(size, vk::BufferUsageFlagBits::eUniformBuffer,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, m_Buffer,
			m_Memory);
		}

		VKUniformBuffer::VKUniformBuffer()
		{
		}

		VKUniformBuffer::~VKUniformBuffer()
		{
		}

		void VKUniformBuffer::Init(uint32_t size, const void* data)
		{
			VKTools::CreateBuffer(size, vk::BufferUsageFlagBits::eUniformBuffer,
				vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, m_Buffer, m_Memory
			);
		}

		void VKUniformBuffer::SetData(uint32_t size, const void* data)
		{
			void* temp;
			VKDevice::Instance()->GetDevice().mapMemory(m_Memory, vk::DeviceSize(0), size, vk::MemoryMapFlagBits(), &temp);
			memcpy(temp, data, static_cast<size_t>(size));
			VKDevice::Instance()->GetDevice().unmapMemory(m_Memory);
		}

		void VKUniformBuffer::SetDynamicData(uint32_t size, uint32_t typeSize, const void* data)
		{
			void* temp;
			VKDevice::Instance()->GetDevice().mapMemory(m_Memory, vk::DeviceSize(0), size, vk::MemoryMapFlagBits(), &temp);
			memcpy(temp, data, size);
			vk::MappedMemoryRange memoryRange;
			memoryRange.memory = m_Memory;
			memoryRange.size = size;
			VKDevice::Instance()->GetDevice().flushMappedMemoryRanges(1, &memoryRange);
			VKDevice::Instance()->GetDevice().unmapMemory(m_Memory);
		}
	}
}
