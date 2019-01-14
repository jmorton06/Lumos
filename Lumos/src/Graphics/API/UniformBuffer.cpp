#include "LM.h"
#include "UniformBuffer.h"

#include "Context.h"

#ifdef LUMOS_RENDER_API_VULKAN 
#include "Platform/GraphicsAPI/Vulkan/VKUniformBuffer.h"
#endif

#ifdef LUMOS_RENDER_API_OPENGL
#include "Platform/GraphicsAPI/OpenGL/GLUniformBuffer.h"
#endif

namespace Lumos
{
	namespace graphics
	{
		namespace api
		{
			UniformBuffer* UniformBuffer::Create()
			{
				switch (graphics::Context::GetRenderAPI())
				{
#ifdef LUMOS_RENDER_API_OPENGL
				case RenderAPI::OPENGL:	return new GLUniformBuffer();
#endif
#ifdef LUMOS_RENDER_API_VULKAN
				case RenderAPI::VULKAN:	return new graphics::VKUniformBuffer();
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
				case RenderAPI::DIRECT3D: return nullptr;
#endif
				}
				return nullptr;
			}

			UniformBuffer* UniformBuffer::Create(uint32_t size, const void* data)
			{
				switch (graphics::Context::GetRenderAPI())
				{
#ifdef LUMOS_RENDER_API_OPENGL
				case RenderAPI::OPENGL:	return new GLUniformBuffer();
#endif
#ifdef LUMOS_RENDER_API_VULKAN
				case RenderAPI::VULKAN:	return new graphics::VKUniformBuffer(size,data);
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
				case RenderAPI::DIRECT3D: return nullptr;
#endif
				}
				return nullptr;
			}
		}
	}
}