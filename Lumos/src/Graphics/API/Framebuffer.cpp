#include "lmpch.h"
#include "Framebuffer.h"
#include "Graphics/API/GraphicsContext.h"

namespace Lumos
{
	namespace Graphics
	{
        Framebuffer*(*Framebuffer::CreateFunc)(const FramebufferInfo&) = nullptr;

		Framebuffer* Framebuffer::Create(const FramebufferInfo& framebufferInfo)
		{
            LUMOS_ASSERT(CreateFunc, "No Framebuffer Create Function");
            
            return CreateFunc(framebufferInfo);
		}
	}
}

