#include "Precompiled.h"
#include "SwapChain.h"

namespace Lumos
{
    namespace Graphics
    {
        SwapChain* (*SwapChain::CreateFunc)(uint32_t, uint32_t) = nullptr;

        SwapChain* SwapChain::Create(uint32_t width, uint32_t height)
        {
            LUMOS_ASSERT(CreateFunc, "No Swapchain Create Function");

            return CreateFunc(width, height);
        }
    }
}
