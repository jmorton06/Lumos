#include "JM.h"
#include "Swapchain.h"

#ifdef JM_RENDER_API_VULKAN
#include "Platform/GraphicsAPI/Vulkan/VKSwapchain.h"
#endif

#ifdef JM_RENDER_API_OPENGL
#include "Platform/GraphicsAPI/OpenGL/GLSwapchain.h"
#endif

#include "Graphics/API/Context.h"

namespace jm
{
	namespace graphics
	{
		namespace api
		{
			Swapchain* Swapchain::Create(uint width, uint height)
			{
				switch (graphics::Context::GetRenderAPI())
				{
#ifdef JM_RENDER_API_OPENGL
				case RenderAPI::OPENGL:		return new GLSwapchain(width, height);
#endif
#ifdef JM_RENDER_API_VULKAN
				case RenderAPI::VULKAN:		return new VKSwapchain(width, height);
#endif
				}
				return nullptr;
			}
		}
	}
}
