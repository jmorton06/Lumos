#include "LM.h"
#include "VKBuffer.h"
#include "VKDevice.h"
#include "VKRenderer.h"

namespace Lumos
{
	namespace graphics
	{
		constexpr uint32_t INVALID_MEMORY_INDEX = ~(0u);

		static uint32_t memoryIndexFromPropertyFlags(
			const vk::PhysicalDeviceMemoryProperties& properties,
			const vk::MemoryRequirements& requirements, vk::MemoryPropertyFlags flags)
		{
			for (uint32_t i = 0; i < properties.memoryTypeCount; ++i)
				if ((requirements.memoryTypeBits & (1 << i)) &&
					((properties.memoryTypes[i].propertyFlags & flags) == flags))
					return i;
			return INVALID_MEMORY_INDEX;
		}

		VKBuffer::VKBuffer(vk::BufferUsageFlags usage, uint32_t size, const void* data)
		{
			Init(usage, size, data);
		}

		VKBuffer::VKBuffer()
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
			vk::BufferCreateInfo buffer_desc{};
			buffer_desc.size = size;
			buffer_desc.usage = vk::BufferUsageFlagBits::eTransferSrc;
			buffer_desc.sharingMode = vk::SharingMode::eExclusive;

			vk::Buffer upload_buffer = VKDevice::Instance()->GetDevice().createBuffer(buffer_desc);

			vk::MemoryRequirements memory_requirements = VKDevice::Instance()->GetDevice().getBufferMemoryRequirements(upload_buffer);
			vk::PhysicalDeviceMemoryProperties memory_properties = VKDevice::Instance()->GetGPU().getMemoryProperties();

			uint32_t memory_index = memoryIndexFromPropertyFlags(memory_properties, 
				memory_requirements, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
			if (memory_index == INVALID_MEMORY_INDEX)
				LUMOS_CORE_ERROR("Invalid memory index");

			vk::MemoryAllocateInfo allocate_info{};
			allocate_info.allocationSize = memory_requirements.size;
			allocate_info.memoryTypeIndex = memory_index;

			vk::DeviceMemory upload_memory = VKDevice::Instance()->GetDevice().allocateMemory(allocate_info);
			VKDevice::Instance()->GetDevice().bindBufferMemory(upload_buffer, upload_memory, 0);

			void* transfer_data = VKDevice::Instance()->GetDevice().mapMemory(upload_memory, 0, size);
			memcpy(transfer_data, data, size);
			VKDevice::Instance()->GetDevice().unmapMemory(upload_memory);

			buffer_desc.usage = vk::BufferUsageFlagBits::eTransferDst | usage;
			m_Buffer = VKDevice::Instance()->GetDevice().createBuffer(buffer_desc);

			memory_requirements = VKDevice::Instance()->GetDevice().getBufferMemoryRequirements(m_Buffer);
			memory_index = memoryIndexFromPropertyFlags(memory_properties, memory_requirements, vk::MemoryPropertyFlagBits::eDeviceLocal);

			if (memory_index == INVALID_MEMORY_INDEX)
				LUMOS_CORE_ERROR("Invalid memory index");

			allocate_info.allocationSize = memory_requirements.size;
			allocate_info.memoryTypeIndex = memory_index;
			
			m_Memory = VKDevice::Instance()->GetDevice().allocateMemory(allocate_info);
			VKDevice::Instance()->GetDevice().bindBufferMemory(m_Buffer, m_Memory, 0);

			const vk::DeviceSize copy_size = static_cast<vk::DeviceSize>(size);
			VKTools::CopyBuffer(upload_buffer, m_Buffer, copy_size);

			m_DesciptorBufferInfo.buffer = m_Buffer;
			m_DesciptorBufferInfo.offset = 0;
			m_DesciptorBufferInfo.range = size;

			VKDevice::Instance()->GetDevice().freeMemory(upload_memory);
			VKDevice::Instance()->GetDevice().destroyBuffer(upload_buffer);
		}

		void VKBuffer::SetData(uint32_t size, const void* data)
		{
			void* temp;
            VKDevice::Instance()->GetDevice().mapMemory(m_Memory, 0, size, vk::MemoryMapFlagBits(), &temp);
			memcpy(temp, data, size);
            VKDevice::Instance()->GetDevice().unmapMemory(m_Memory);
		}
	}
}
