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
		UniformBuffer* UniformBuffer::Create()
		{
			switch (Graphics::GraphicsContext::GetRenderAPI())
			{
#ifdef LUMOS_RENDER_API_OPENGL
			case RenderAPI::OPENGL:	return lmnew GLUniformBuffer();
#endif
#ifdef LUMOS_RENDER_API_VULKAN
			case RenderAPI::VULKAN:	return lmnew Graphics::VKUniformBuffer();
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
			case RenderAPI::DIRECT3D: return nullptr;
#endif
			}
			return nullptr;
		}

		UniformBuffer* UniformBuffer::Create(uint32_t size, const void* data)
		{
			switch (Graphics::GraphicsContext::GetRenderAPI())
			{
#ifdef LUMOS_RENDER_API_OPENGL
			case RenderAPI::OPENGL:	return lmnew GLUniformBuffer();
#endif
#ifdef LUMOS_RENDER_API_VULKAN
			case RenderAPI::VULKAN:	return lmnew Graphics::VKUniformBuffer(size, data);
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
			case RenderAPI::DIRECT3D: return nullptr;
#endif
			}
			return nullptr;
		}
	}
}