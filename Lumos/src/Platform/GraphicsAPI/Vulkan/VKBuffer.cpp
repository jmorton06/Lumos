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
			const VkPhysicalDeviceMemoryProperties& properties,
			const VkMemoryRequirements& requirements, uint32_t flags)
		{
			for (uint32_t i = 0; i < properties.memoryTypeCount; ++i)
				if ((requirements.memoryTypeBits & (1 << i)) &&
					((properties.memoryTypes[i].propertyFlags & flags) == flags))
					return i;
			return INVALID_MEMORY_INDEX;
		}

		VKBuffer::VKBuffer(VkBufferUsageFlags usage, uint32_t size, const void* data)
		{
			Init(usage, size, data);
		}

		VKBuffer::VKBuffer()
		{
		}

		VKBuffer::~VKBuffer()
		{
			vkDestroyBuffer(VKDevice::Instance()->GetDevice(), m_Buffer, nullptr);
			vkFreeMemory(VKDevice::Instance()->GetDevice(), m_Memory, nullptr);
		}

		void VKBuffer::Init(VkBufferUsageFlags usage, uint32_t size, const void* data)
		{
			VkBufferCreateInfo buffer_desc{};
			buffer_desc.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			buffer_desc.size = size;
			buffer_desc.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			buffer_desc.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			VkBuffer upload_buffer;
			vkCreateBuffer(VKDevice::Instance()->GetDevice(), &buffer_desc, nullptr, &upload_buffer);

			VkMemoryRequirements memory_requirements;
			vkGetBufferMemoryRequirements(VKDevice::Instance()->GetDevice(), upload_buffer,
				&memory_requirements);

			VkPhysicalDeviceMemoryProperties memory_properties;
			vkGetPhysicalDeviceMemoryProperties(VKDevice::Instance()->GetGPU(),
				&memory_properties);

			uint32_t memory_index = memoryIndexFromPropertyFlags(memory_properties,
				memory_requirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			if (memory_index == INVALID_MEMORY_INDEX)
				throw std::runtime_error("Invalid memory index");

			VkMemoryAllocateInfo allocate_info{};
			allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocate_info.allocationSize = memory_requirements.size;
			allocate_info.memoryTypeIndex = memory_index;

			VkDeviceMemory upload_memory;
			vkAllocateMemory(VKDevice::Instance()->GetDevice(), &allocate_info,
				nullptr, &upload_memory);

			vkBindBufferMemory(VKDevice::Instance()->GetDevice(), upload_buffer,
				upload_memory, 0);

			void* transfer_data = nullptr;
			vkMapMemory(VKDevice::Instance()->GetDevice(), upload_memory, 0,
				size, 0, &transfer_data);

			memcpy(transfer_data, data, size);

			vkUnmapMemory(VKDevice::Instance()->GetDevice(), upload_memory);

			buffer_desc.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage;
			vkCreateBuffer(VKDevice::Instance()->GetDevice(), &buffer_desc,
				nullptr, &m_Buffer);

			vkGetBufferMemoryRequirements(VKDevice::Instance()->GetDevice(), m_Buffer,
				&memory_requirements);

			memory_index = memoryIndexFromPropertyFlags(memory_properties,
				memory_requirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			if (memory_index == INVALID_MEMORY_INDEX)
				throw std::runtime_error("Invalid memory index");

			allocate_info.allocationSize = memory_requirements.size;
			allocate_info.memoryTypeIndex = memory_index;

			vkAllocateMemory(VKDevice::Instance()->GetDevice(), &allocate_info,
				nullptr, &m_Memory);

			vkBindBufferMemory(VKDevice::Instance()->GetDevice(), m_Buffer,
				m_Memory, 0);

			const VkDeviceSize copy_size = static_cast<VkDeviceSize>(size);
			VKTools::copyBuffer(upload_buffer, m_Buffer, copy_size);

			m_DesciptorBufferInfo.buffer = m_Buffer;
			m_DesciptorBufferInfo.offset = 0;
			m_DesciptorBufferInfo.range = size;

			vkFreeMemory(VKDevice::Instance()->GetDevice(), upload_memory, nullptr);
			vkDestroyBuffer(VKDevice::Instance()->GetDevice(), upload_buffer, nullptr);
		}

		void VKBuffer::SetData(uint32_t size, const void* data)
		{
			void* temp;
			vkMapMemory(VKDevice::Instance()->GetDevice(), m_Memory, 0, size, 0, &temp);
			memcpy(temp, data, size);
			vkUnmapMemory(VKDevice::Instance()->GetDevice(), m_Memory);
		}
	}
}
