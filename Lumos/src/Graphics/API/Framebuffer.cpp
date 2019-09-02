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
        Framebuffer*(*Framebuffer::CreateFunc)(const FramebufferInfo&) = nullptr;

		Framebuffer* Framebuffer::Create(const FramebufferInfo& framebufferInfo)
		{
            LUMOS_CORE_ASSERT(CreateFunc, "No Framebuffer Create Function");
            
#ifdef LUMOS_RENDER_API_OPENGL
			if(Graphics::GraphicsContext::GetRenderAPI() == RenderAPI::OPENGL && framebufferInfo.screenFBO)
				return nullptr; //TODO: REMOVE
#endif
            return CreateFunc(framebufferInfo);
		}
	}
}

