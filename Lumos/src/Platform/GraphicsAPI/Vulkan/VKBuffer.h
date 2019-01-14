#pragma once

#include "Dependencies/vulkan/vulkan.h"

namespace jm
{
	namespace graphics
	{
		class VKBuffer
		{
		public:
			VKBuffer(VkBufferUsageFlags usage, uint32_t size, const void* data);
			VKBuffer();
			virtual ~VKBuffer();

			void Init(VkBufferUsageFlags usage, uint32_t size, const void* data);

			void SetData(uint32_t size, const void* data);
			VkBuffer GetBuffer() const { return m_Buffer; }

			VkDescriptorBufferInfo GetBufferInfo() const { return m_DesciptorBufferInfo; };


		protected:
			VkBuffer m_Buffer{};
			VkDeviceMemory m_Memory{};
			VkDescriptorBufferInfo m_DesciptorBufferInfo;
		};
	}
}
