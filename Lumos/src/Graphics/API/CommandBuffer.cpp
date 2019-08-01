#include "LM.h"
#include "CommandBuffer.h"
#include "GraphicsContext.h"

#ifdef LUMOS_RENDER_API_VULKAN
#include "Platform/Vulkan/VKCommandBuffer.h"
#endif

#ifdef LUMOS_RENDER_API_OPENGL
#include "Platform/OpenGL/GLCommandBuffer.h"
#endif

namespace Lumos
{
	namespace Graphics
	{
		CommandBuffer* CommandBuffer::Create()
		{
			switch (Graphics::GraphicsContext::GetRenderAPI())
			{
#ifdef LUMOS_RENDER_API_OPENGL
			case RenderAPI::OPENGL:		return lmnew GLCommandBuffer();
#endif
#ifdef LUMOS_RENDER_API_VULKAN
			case RenderAPI::VULKAN:		return lmnew VKCommandBuffer();
#endif
			}
			return nullptr;
		}
	}
}
