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
    }
}
