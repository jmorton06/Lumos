#include "LM.h"
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
		RenderPass* RenderPass::Create()
		{
			switch (Graphics::GraphicsContext::GetRenderAPI())
			{
#ifdef LUMOS_RENDER_API_OPENGL
			case RenderAPI::OPENGL:		return new GLRenderPass();
#endif
#ifdef LUMOS_RENDER_API_VULKAN
			case RenderAPI::VULKAN:		return new VKRenderpass();
#endif
			}
			return nullptr;
		}
	}
}
