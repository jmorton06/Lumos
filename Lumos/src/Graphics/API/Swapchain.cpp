#include "LM.h"
#include "Swapchain.h"

#ifdef LUMOS_RENDER_API_VULKAN
#include "Platform/Vulkan/VKSwapchain.h"
#endif

#ifdef LUMOS_RENDER_API_OPENGL
#include "Platform/OpenGL/GLSwapchain.h"
#endif

#include "Graphics/API/GraphicsContext.h"

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
