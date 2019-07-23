#include "LM.h"
#include "GLSwapchain.h"
#include "Graphics/API/Framebuffer.h"
#include "GLTexture.h"
#include "App/Window.h"

namespace Lumos
{
	namespace Graphics
	{
		GLSwapchain::GLSwapchain(u32 width, u32 height)
		{
			FramebufferInfo info{};
			info.width = width;
			info.height = height;
			info.attachments = nullptr;
		}

		GLSwapchain::~GLSwapchain()
		{
			for (auto& buffer : swapChainBuffers)
				delete buffer;
		}

		bool GLSwapchain::Init()
		{
			return true;
		}

		Texture* GLSwapchain::GetCurrentImage()
		{
			return nullptr;//swapChainBuffers[0];
		}

		uint32_t GLSwapchain::GetCurrentBufferId() const
		{
			return 0;
		}

		size_t GLSwapchain::GetSwapchainBufferCount() const
		{
			return 1;
		}
	}
}

