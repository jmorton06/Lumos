#include "lmpch.h"
#include "RenderPass.h"
#include "GraphicsContext.h"

#ifdef LUMOS_RENDER_API_VULKAN
#include "Platform/Vulkan/VKRenderpass.h"
#endif

#ifdef LUMOS_RENDER_API_OPENGL
#include "Platform/OpenGL/GLRenderPass.h"
#endif

namespace Lumos
{
	namespace Graphics
	{
        RenderPass*(*RenderPass::CreateFunc)() = nullptr;

		RenderPass* RenderPass::Create()
		{
            LUMOS_ASSERT(CreateFunc, "No RenderPass Create Function");
            
            return CreateFunc();
		}
	}
}
