#include "Precompiled.h"
#include "StorageBuffer.h"

namespace Lumos
{
    namespace Graphics
    {
        StorageBuffer* (*StorageBuffer::CreateFunc)(uint32_t, const void*) = nullptr;

        StorageBuffer* StorageBuffer::Create(uint32_t size, const void* data)
        {
            ASSERT(CreateFunc, "No StorageBuffer Create Function");
            return CreateFunc(size, data);
        }
    }
}
