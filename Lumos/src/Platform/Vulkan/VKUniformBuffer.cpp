#include "LM.h"
#include "VKUniformBuffer.h"
#include "VKDevice.h"
#include "VKTools.h"

namespace Lumos
{
	namespace Graphics
	{
		VKUniformBuffer::VKUniformBuffer(uint32_t size, const void* data)
		{
			VKBuffer::Init(vk::BufferUsageFlagBits::eUniformBuffer, size, data);
		}

		VKUniformBuffer::VKUniformBuffer()
		{
		}

		VKUniformBuffer::~VKUniformBuffer()
		{
		}

		void VKUniformBuffer::Init(uint32_t size, const void* data)
		{
			VKBuffer::Init(vk::BufferUsageFlagBits::eUniformBuffer, size, data);
		}

		void VKUniformBuffer::SetData(uint32_t size, const void* data)
		{
			VKBuffer::Map();
			memcpy(m_Mapped, data, static_cast<size_t>(size));
			VKBuffer::UnMap();
		}

		void VKUniformBuffer::SetDynamicData(uint32_t size, uint32_t typeSize, const void* data)
		{
			VKBuffer::Map();
			memcpy(m_Mapped, data, size);

			vk::MappedMemoryRange memoryRange;
			memoryRange.memory = m_Memory;
			memoryRange.size = size;
			VKDevice::Instance()->GetDevice().flushMappedMemoryRanges(1, &memoryRange);

			VKBuffer::UnMap();
		}
	}
}
