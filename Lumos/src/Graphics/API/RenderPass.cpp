#include "JM.h"
#include "RenderPass.h"
#include "Context.h"

#ifdef JM_RENDER_API_VULKAN
#include "Platform/GraphicsAPI/Vulkan/VKRenderpass.h"
#endif

#ifdef JM_RENDER_API_OPENGL
#include "Platform/GraphicsAPI/OpenGL/GLRenderPass.h"
#endif

namespace jm
{
	namespace graphics
	{
		namespace api
		{
			RenderPass* RenderPass::Create()
			{
				switch (graphics::Context::GetRenderAPI())
				{
#ifdef JM_RENDER_API_OPENGL
				case RenderAPI::OPENGL:		return new GLRenderPass();
#endif
#ifdef JM_RENDER_API_VULKAN
				case RenderAPI::VULKAN:		return new VKRenderpass();
#endif
				}
			return nullptr;
			}
		}
	}
}
