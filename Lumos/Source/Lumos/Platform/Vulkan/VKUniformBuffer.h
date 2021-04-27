#pragma once
#include "VK.h"
#include "VKBuffer.h"
#include "Graphics/API/UniformBuffer.h"

#ifdef USE_VMA_ALLOCATOR
#include <vulkan/vk_mem_alloc.h>
#endif

namespace Lumos
{
    namespace Graphics
    {
        class VKUniformBuffer : public UniformBuffer, public VKBuffer
        {
        public:
            VKUniformBuffer(uint32_t size, const void* data);
            VKUniformBuffer();
            ~VKUniformBuffer();

            void Init(uint32_t size, const void* data) override;

            void SetData(uint32_t size, const void* data) override;
            void SetDynamicData(uint32_t size, uint32_t typeSize, const void* data) override;

            VkBuffer* GetBuffer() { return &m_Buffer; }
            VkDeviceMemory* GetMemory() { return &m_Memory; }
            const VkDescriptorBufferInfo& GetBufferInfo() const { return m_DesciptorBufferInfo; };

            uint8_t* GetBuffer() const override { return nullptr; };
            static void MakeDefault();

        protected:
            static UniformBuffer* CreateFuncVulkan();
            static UniformBuffer* CreateDataFuncVulkan(uint32_t, const void*);

#ifdef USE_VMA_ALLOCATOR
            VmaAllocation m_Allocation;
#endif
        };
    }
}
