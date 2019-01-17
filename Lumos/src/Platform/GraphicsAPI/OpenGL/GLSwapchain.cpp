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
			frameBuffers.push_back(Framebuffer::Create(info));
			swapChainBuffers.push_back(new GLTexture2D(width,height));
		}

		GLSwapchain::~GLSwapchain()
		{
			for (auto& framebuffer : frameBuffers)
				delete framebuffer;

			for (auto& buffer : swapChainBuffers)
				delete buffer;
		}

		bool GLSwapchain::Init(TextureDepth* depthTexture, api::RenderPass* vulkanRenderpass)
		{
			return true;
		}

		Texture* GLSwapchain::GetCurrentImage()
		{
			return swapChainBuffers[0];
		}

		uint32_t GLSwapchain::GetCurrentBufferId() const
		{
			return 0;
		}

		Framebuffer* GLSwapchain::GetFramebuffer(int id) const
		{
			return nullptr;// static_cast<int>(frameBuffers.size()) >= id ? frameBuffers[id] : nullptr;
		}

		size_t GLSwapchain::GetSwapchainBufferCount() const
		{
			return frameBuffers.size();
		}
	}
}

