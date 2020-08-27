#include "Precompiled.h"
#include "Swapchain.h"

namespace Lumos
{
	namespace Graphics
	{
        Swapchain*(*Swapchain::CreateFunc)(u32, u32) = nullptr;

		Swapchain* Swapchain::Create(u32 width, u32 height)
		{
            LUMOS_ASSERT(CreateFunc, "No Swapchain Create Function");
            
            return CreateFunc(width, height);
		}
	}
}
