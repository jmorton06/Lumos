#include "JM.h"
#include "FrameBuffer.h"
#include "Graphics/API/Context.h"

#ifdef JM_RENDER_API_VULKAN
#include "Platform/GraphicsAPI/Vulkan/VKFramebuffer.h"
#endif
#ifdef JM_RENDER_API_OPENGL
#include "Platform/GraphicsAPI/OpenGL/GLFrameBuffer.h"
#endif
#ifdef JM_RENDER_API_DIRECT3D
#include "graphics/DirectX/DXFrameBuffer2D.h"
#endif
namespace jm
{

	Framebuffer* Framebuffer::Create(FrameBufferInfo framebufferInfo)
	{
		switch (graphics::Context::GetRenderAPI())
		{
#ifdef JM_RENDER_API_OPENGL
		case RenderAPI::OPENGL:	return new GLFramebuffer(framebufferInfo);
#endif
#ifdef JM_RENDER_API_VULKAN
		case RenderAPI::VULKAN:	return new graphics::VKFrameBuffer(framebufferInfo);
#endif
#ifdef JM_RENDER_API_DIRECT3D
		case RenderAPI::DIRECT3D: return new D3DFrameBuffer2D();
#endif
		}
		return nullptr;
	}
}

