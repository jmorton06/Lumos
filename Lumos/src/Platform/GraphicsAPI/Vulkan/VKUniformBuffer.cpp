#include "JM.h"
#include "VKUniformBuffer.h"
#include "VKDevice.h"
#include "VKTools.h"
#include "VKInitialisers.h"

namespace jm
{
	namespace graphics
	{
		VKUniformBuffer::VKUniformBuffer(uint32_t size, const void* data)
		{
			VKTools::CreateBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_Buffer,
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
			VKTools::CreateBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_Buffer, m_Memory
			);
		}

		void VKUniformBuffer::SetData(uint32_t size, const void* data)
		{
			void* temp;
			vkMapMemory(VKDevice::Instance()->GetDevice(), m_Memory, 0, size, 0, &temp);
			memcpy(temp, data, size);
			vkUnmapMemory(VKDevice::Instance()->GetDevice(), m_Memory);
		}

		void VKUniformBuffer::SetDynamicData(uint32_t size, uint32_t typeSize, const void* data)
		{
			void* temp;
			vkMapMemory(VKDevice::Instance()->GetDevice(), m_Memory, 0, size, 0, &temp);
			memcpy(temp, data, size);
			// Flush to make changes visible to the host
			VkMappedMemoryRange memoryRange = initializers::mappedMemoryRange();
			memoryRange.memory = m_Memory;
			memoryRange.size = size;
			vkFlushMappedMemoryRanges(VKDevice::Instance()->GetDevice(), 1, &memoryRange);
			vkUnmapMemory(VKDevice::Instance()->GetDevice(), m_Memory);
		}
	}
}
