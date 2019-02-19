#include "LM.h"
#include "GLSwapchain.h"
#include "Graphics/API/Framebuffer.h"
#include "App/Window.h"

namespace Lumos
{
	namespace graphics
	{
		GLSwapchain::GLSwapchain(uint width, uint height)
		{
			FramebufferInfo info{};
			info.width = width;
			info.height = height;
			info.attachments = nullptr;
			swapChainBuffers.push_back(new GLTexture2D(width,height));
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

