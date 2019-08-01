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
		Swapchain* Swapchain::Create(u32 width, u32 height)
		{
			switch (Graphics::GraphicsContext::GetRenderAPI())
			{
#ifdef LUMOS_RENDER_API_OPENGL
			case RenderAPI::OPENGL:		return lmnew GLSwapchain(width, height);
#endif
#ifdef LUMOS_RENDER_API_VULKAN
			case RenderAPI::VULKAN:		return lmnew VKSwapchain(width, height);
#endif
			}
			return nullptr;
		}
	}
}
