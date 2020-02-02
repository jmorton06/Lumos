#include "lmpch.h"
#include "Framebuffer.h"

namespace Lumos
{
	namespace Graphics
	{
        Framebuffer*(*Framebuffer::CreateFunc)(const FramebufferInfo&) = nullptr;

		Framebuffer* Framebuffer::Create(const FramebufferInfo& framebufferInfo)
		{
            LUMOS_ASSERT(CreateFunc, "No Framebuffer Create Function");
            
#ifdef LUMOS_RENDER_API_OPENGL
			if(Graphics::GraphicsContext::GetRenderAPI() == RenderAPI::OPENGL && framebufferInfo.screenFBO)
				return nullptr; //TODO: REMOVE
#endif
            return CreateFunc(framebufferInfo);
		}
	}
}

