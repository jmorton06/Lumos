#include "LM.h"
#include "Framebuffer.h"
#include "Graphics/API/Context.h"

#ifdef LUMOS_RENDER_API_VULKAN
#include "Platform/GraphicsAPI/Vulkan/VKFramebuffer.h"
#endif
#ifdef LUMOS_RENDER_API_OPENGL
#include "Platform/GraphicsAPI/OpenGL/GLFrameBuffer.h"
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
#include "graphics/DirectX/DXFrameBuffer2D.h"
#endif
namespace Lumos
{

	Framebuffer* Framebuffer::Create(FramebufferInfo framebufferInfo)
	{
		switch (graphics::Context::GetRenderAPI())
		{
#ifdef LUMOS_RENDER_API_OPENGL
		case RenderAPI::OPENGL:	return new GLFramebuffer(framebufferInfo);
#endif
#ifdef LUMOS_RENDER_API_VULKAN
		case RenderAPI::VULKAN:	return new graphics::VKFramebuffer(framebufferInfo);
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
		case RenderAPI::DIRECT3D: return new D3DFrameBuffer2D();
#endif
		}
		return nullptr;
	}
}

