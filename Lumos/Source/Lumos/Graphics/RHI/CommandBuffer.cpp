#ifndef LUMOS_PLATFORM_MACOS
#include "Precompiled.h"
#endif
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
