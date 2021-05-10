#pragma once

#include "Graphics/API/IndexBuffer.h"
#include "GLDebug.h"

namespace Lumos
{
    namespace Graphics
    {
        class GLIndexBuffer : public IndexBuffer
        {
        private:
            uint32_t m_Handle;
            uint32_t m_Count;
            BufferUsage m_Usage;
            bool m_Mapped = false;

        public:
            GLIndexBuffer(uint16_t* data, uint32_t count, BufferUsage bufferUsage);
            GLIndexBuffer(uint32_t* data, uint32_t count, BufferUsage bufferUsage);
            ~GLIndexBuffer();

            void Bind(CommandBuffer* commandBuffer) const override;
            void Unbind() const override;
            uint32_t GetCount() const override;
            void SetCount(uint32_t m_index_count) override { m_Count = m_index_count; };

            void* GetPointerInternal() override;
            void ReleasePointer() override;

            static void MakeDefault();

        protected:
            static IndexBuffer* CreateFuncGL(uint32_t* data, uint32_t count, BufferUsage bufferUsage);
            static IndexBuffer* CreateFunc16GL(uint16_t* data, uint32_t count, BufferUsage bufferUsage);
        };
    }
}
