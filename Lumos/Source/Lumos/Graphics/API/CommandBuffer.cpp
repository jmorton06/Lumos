#include "Precompiled.h"
#include "CommandBuffer.h"

namespace Lumos
{
    namespace Graphics
    {
        CommandBuffer* (*CommandBuffer::CreateFunc)() = nullptr;

        CommandBuffer* CommandBuffer::Create()
        {
            LUMOS_ASSERT(CreateFunc, "No CommandBuffer Create Function");

            return CreateFunc();
        }
    }
}
