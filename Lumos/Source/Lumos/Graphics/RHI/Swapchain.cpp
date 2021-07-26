#include "Precompiled.h"
#include "Swapchain.h"

namespace Lumos
{
    namespace Graphics
    {
        Swapchain* (*Swapchain::CreateFunc)(uint32_t, uint32_t) = nullptr;

        Swapchain* Swapchain::Create(uint32_t width, uint32_t height)
        {
            LUMOS_ASSERT(CreateFunc, "No Swapchain Create Function");

            return CreateFunc(width, height);
        }
    }
}
