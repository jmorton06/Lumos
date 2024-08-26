#include "Precompiled.h"
#include "CommandBuffer.h"

namespace Lumos
{
    namespace Graphics
    {
        CommandBuffer* (*CommandBuffer::CreateFunc)() = nullptr;

        CommandBuffer* CommandBuffer::Create()
        {
            ASSERT(CreateFunc, "No CommandBuffer Create Function");

            return CreateFunc();
        }
    }
}
