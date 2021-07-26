#pragma once

#include "Graphics/RHI/VertexBuffer.h"

namespace Lumos
{
    namespace Graphics
    {
        class CommandBuffer;

        class LUMOS_EXPORT IndexBuffer
        {
        public:
            virtual ~IndexBuffer() = default;
            virtual void Bind(CommandBuffer* commandBuffer = nullptr) const = 0;
            virtual void Unbind() const = 0;

            virtual uint32_t GetCount() const = 0;
            virtual uint32_t GetSize() const { return 0; }
            virtual void SetCount(uint32_t m_index_count) = 0;
            virtual void ReleasePointer() {};

            template <typename T>
            T* GetPointer()
            {
                return static_cast<T*>(GetPointerInternal());
            }

        public:
            static IndexBuffer* Create(uint16_t* data, uint32_t count, BufferUsage bufferUsage = BufferUsage::STATIC);
            static IndexBuffer* Create(uint32_t* data, uint32_t count, BufferUsage bufferUsage = BufferUsage::STATIC);

        protected:
            virtual void* GetPointerInternal() { return nullptr; }

            static IndexBuffer* (*Create16Func)(uint16_t*, uint32_t, BufferUsage);
            static IndexBuffer* (*CreateFunc)(uint32_t*, uint32_t, BufferUsage);
        };
    }
}
