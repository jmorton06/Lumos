#pragma once

#include "VK.h"

namespace Lumos
{
	namespace graphics
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


		protected:
			vk::Buffer m_Buffer{};
			vk::DeviceMemory m_Memory{};
			vk::DescriptorBufferInfo m_DesciptorBufferInfo;
		};
	}
}
