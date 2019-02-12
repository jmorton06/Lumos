#include "LM.h"
#include "CommandBuffer.h"
#include "Context.h"

#ifdef LUMOS_RENDER_API_VULKAN
#include "Platform/Vulkan/VKCommandBuffer.h"
#endif

#ifdef LUMOS_RENDER_API_OPENGL
#include "Platform/OpenGL/GLCommandBuffer.h"
#endif

namespace Lumos
{
	namespace graphics
	{
		namespace api
		{
			CommandBuffer* CommandBuffer::Create()
			{
				switch (graphics::Context::GetRenderAPI())
				{
#ifdef LUMOS_RENDER_API_OPENGL
				case RenderAPI::OPENGL:		return new GLCommandBuffer();
#endif
#ifdef LUMOS_RENDER_API_VULKAN
				case RenderAPI::VULKAN:		return new VKCommandBuffer();
#endif
				}
				return nullptr;
			}
		}
	}
}
