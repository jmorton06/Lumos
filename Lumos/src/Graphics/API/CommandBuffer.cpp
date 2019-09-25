#include "lmpch.h"
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
        CommandBuffer*(*CommandBuffer::CreateFunc)() = nullptr;

		CommandBuffer* CommandBuffer::Create()
		{
            LUMOS_ASSERT(CreateFunc, "No CommandBuffer Create Function");
            
            return CreateFunc();
		}
	}
}
