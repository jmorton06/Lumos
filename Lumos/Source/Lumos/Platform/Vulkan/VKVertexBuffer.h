#pragma once

#include "VKBuffer.h"
#include "Graphics/API/VertexBuffer.h"

namespace Lumos
{
    namespace Graphics
    {
        class VKVertexBuffer : public VertexBuffer, public VKBuffer
        {
        public:
            explicit VKVertexBuffer(const BufferUsage& usage);
            ~VKVertexBuffer();

            void Resize(uint32_t size) override;
            void SetData(uint32_t size, const void* data) override;
            void SetDataSub(uint32_t size, const void* data, uint32_t offset) override;
            void ReleasePointer() override;

            void Bind(CommandBuffer* commandBuffer, Pipeline* pipeline) override;
            void Unbind() override;
            uint32_t GetSize() override { return m_Size; }

            static void MakeDefault();

        protected:
            static VertexBuffer* CreateFuncVulkan(const BufferUsage& usage);
            void* GetPointerInternal() override;

            bool m_MappedBuffer = false;

            BufferUsage m_Usage;
            uint32_t m_Size;
        };

    }
}
