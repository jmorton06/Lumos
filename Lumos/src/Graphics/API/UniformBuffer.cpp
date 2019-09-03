#include "LM.h"
#include "UniformBuffer.h"

#include "GraphicsContext.h"

#ifdef LUMOS_RENDER_API_VULKAN 
#include "Platform/Vulkan/VKUniformBuffer.h"
#endif

#ifdef LUMOS_RENDER_API_OPENGL
#include "Platform/OpenGL/GLUniformBuffer.h"
#endif

namespace Lumos
{
	namespace Graphics
	{
        UniformBuffer* (*UniformBuffer::CreateFunc)() = nullptr;
        UniformBuffer* (*UniformBuffer::CreateDataFunc)(uint32_t, const void*) = nullptr;

		UniformBuffer* UniformBuffer::Create()
		{
            LUMOS_ASSERT(CreateFunc, "No UniformBuffer Create Function");
            
            return CreateFunc();
		}

		UniformBuffer* UniformBuffer::Create(uint32_t size, const void* data)
		{
            LUMOS_ASSERT(CreateFunc, "No UniformBuffer Create Function");
            
            return CreateDataFunc(size, data);
		}
	}
}
