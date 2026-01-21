#pragma once
#include "VK.h"
#include "VKBuffer.h"
#include "Graphics/RHI/StorageBuffer.h"

namespace Lumos
{
    namespace Graphics
    {
        class VKStorageBuffer : public StorageBuffer, public VKBuffer
        {
        public:
            VKStorageBuffer(uint32_t size, const void* data);
            VKStorageBuffer();
            ~VKStorageBuffer();

            void SetData(uint32_t size, const void* data) override;
            void Resize(uint32_t size, const void* data) override;
            void Unmap() override;

            void* GetBuffer() const override { return (void*)&m_Buffer; }
            uint32_t GetSize() const override { return (uint32_t)m_Size; }

            const VkDescriptorBufferInfo& GetBufferInfo() const { return m_DesciptorBufferInfo; }

            static void MakeDefault();

        protected:
            void* GetPointerInternal() override;
            static StorageBuffer* CreateFuncVulkan(uint32_t size, const void* data);
        };
    }
}
