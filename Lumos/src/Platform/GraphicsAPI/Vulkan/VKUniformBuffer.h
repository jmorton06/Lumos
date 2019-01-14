#pragma once
#include "Dependencies/vulkan/vulkan.h"
#include "VKBuffer.h"
#include "Graphics/API/UniformBuffer.h"

namespace jm
{
	namespace graphics
	{
		class VKUniformBuffer : public api::UniformBuffer
		{
		public:
			VKUniformBuffer(uint32_t size, const void* data);
			VKUniformBuffer();
			~VKUniformBuffer();

			void Init(uint32_t size, const void* data) override;

			void SetData(uint32_t size, const void* data) override;
			void SetDynamicData(uint32_t size,  uint32_t typeSize, const void* data) override;
			VkBuffer* GetBuffer() { return &m_Buffer; }

			VkDescriptorBufferInfo GetBufferInfo() const { return m_DesciptorBufferInfo; };
			VkDeviceMemory* GetMemory() { return &m_Memory; }

			byte* GetBuffer() const override { return nullptr; };

		protected:
			VkBuffer m_Buffer{};
			VkDeviceMemory m_Memory{};
			VkDescriptorBufferInfo m_DesciptorBufferInfo;
		};
	}
}
