#pragma once
#include "VK.h"
#include "VKBuffer.h"
#include "Graphics/API/UniformBuffer.h"

namespace Lumos
{
	namespace Graphics
	{
		class VKUniformBuffer : public UniformBuffer
		{
		public:
			VKUniformBuffer(uint32_t size, const void* data);
			VKUniformBuffer();
			~VKUniformBuffer();

			void Init(uint32_t size, const void* data) override;

			void SetData(uint32_t size, const void* data) override;
			void SetDynamicData(uint32_t size,  uint32_t typeSize, const void* data) override;

			vk::Buffer* GetBuffer() { return &m_Buffer; }
			vk::DescriptorBufferInfo GetBufferInfo() const { return m_DesciptorBufferInfo; };
			vk::DeviceMemory* GetMemory() { return &m_Memory; }

			u8* GetBuffer() const override { return nullptr; };

		protected:
			vk::Buffer m_Buffer{};
			vk::DeviceMemory m_Memory{};
			vk::DescriptorBufferInfo m_DesciptorBufferInfo;
		};
	}
}
