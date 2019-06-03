#include "LM.h"
#include "Framebuffer.h"
#include "Graphics/API/GraphicsContext.h"

#ifdef LUMOS_RENDER_API_VULKAN
#include "Platform/Vulkan/VKFramebuffer.h"
#endif
#ifdef LUMOS_RENDER_API_OPENGL
#include "Platform/OpenGL/GLFramebuffer.h"
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
#include "Graphics/DirectX/DXFrameBuffer2D.h"
#endif
namespace Lumos
{
	namespace Graphics
	{
		Framebuffer* Framebuffer::Create(const FramebufferInfo& framebufferInfo)
		{
			switch (Graphics::GraphicsContext::GetRenderAPI())
			{
#ifdef LUMOS_RENDER_API_OPENGL
			case RenderAPI::OPENGL: return framebufferInfo.screenFBO ? nullptr : new GLFramebuffer(framebufferInfo); //TODO: REMOVE
#endif
#ifdef LUMOS_RENDER_API_VULKAN
			case RenderAPI::VULKAN:	return new Graphics::VKFramebuffer(framebufferInfo);
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
			case RenderAPI::DIRECT3D: return new D3DFrameBuffer2D();
#endif
			}
			return nullptr;
		}
	}
}

