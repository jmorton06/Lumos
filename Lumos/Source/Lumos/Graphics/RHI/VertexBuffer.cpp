#include "Precompiled.h"
#include "VertexBuffer.h"

namespace Lumos
{
    namespace Graphics
    {
        VertexBuffer* (*VertexBuffer::CreateFunc)(const BufferUsage&) = nullptr;

        VertexBuffer* VertexBuffer::Create(const BufferUsage& usage)
        {
            LUMOS_ASSERT(CreateFunc, "No VertexBuffer Create Function");
            return CreateFunc(usage);
        }

        VertexBuffer* (*VertexBuffer::CreateWithDataFunc)(uint32_t, const void*, const BufferUsage&) = nullptr;

        VertexBuffer* VertexBuffer::Create(uint32_t size, const void* data, const BufferUsage& usage)
        {
            LUMOS_ASSERT(CreateWithDataFunc, "No VertexBuffer Create Function");
            return CreateWithDataFunc(size, data, usage);
        }
    }
}
