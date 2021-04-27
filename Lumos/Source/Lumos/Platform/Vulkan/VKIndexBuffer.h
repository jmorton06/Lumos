#pragma once

#include "VKBuffer.h"
#include "Graphics/API/IndexBuffer.h"

namespace Lumos
{
    namespace Graphics
    {
        class VKIndexBuffer : public IndexBuffer, public VKBuffer
        {
        public:
            VKIndexBuffer(uint16_t* data, uint32_t count, BufferUsage bufferUsage);
            VKIndexBuffer(uint32_t* data, uint32_t count, BufferUsage bufferUsage);
            ~VKIndexBuffer();

            void Bind(CommandBuffer* commandBuffer) const override;
            void Unbind() const override;
            uint32_t GetCount() const override;
            uint32_t GetSize() const override;
            void SetCount(uint32_t m_index_count) override { m_Count = m_index_count; };
            void* GetPointerInternal() override;
            void ReleasePointer() override;

            static void MakeDefault();

        protected:
            static IndexBuffer* CreateFuncVulkan(uint32_t* data, uint32_t count, BufferUsage bufferUsage);
            static IndexBuffer* CreateFunc16Vulkan(uint16_t* data, uint32_t count, BufferUsage bufferUsage);

        private:
            BufferUsage m_Usage;
            uint32_t m_Count;
            uint32_t m_Size;
            bool m_MappedBuffer = false;
        };
    }
}
