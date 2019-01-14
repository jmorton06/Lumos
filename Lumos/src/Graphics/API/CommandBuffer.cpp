#include "JM.h"
#include "CommandBuffer.h"
#include "Context.h"

#ifdef JM_RENDER_API_VULKAN
#include "Platform/GraphicsAPI/Vulkan/VKCommandBuffer.h"
#endif

#ifdef JM_RENDER_API_OPENGL
#include "Platform/GraphicsAPI/OpenGL/GLCommandBuffer.h"
#endif

namespace jm
{
	namespace graphics
	{
		namespace api
		{
			CommandBuffer* CommandBuffer::Create()
			{
				switch (graphics::Context::GetRenderAPI())
				{
#ifdef JM_RENDER_API_OPENGL
				case RenderAPI::OPENGL:		return new GLCommandBuffer();
#endif
#ifdef JM_RENDER_API_VULKAN
				case RenderAPI::VULKAN:		return new VKCommandBuffer();
#endif
				}
				return nullptr;
			}
		}
	}
}
