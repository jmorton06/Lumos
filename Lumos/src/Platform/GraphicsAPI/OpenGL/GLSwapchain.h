#pragma once
#include "LM.h"
#include "Graphics/API/Swapchain.h"
#include "Textures/GLTexture2D.h"

namespace Lumos
{
	namespace graphics
	{
		class GLSwapchain : public api::Swapchain
		{
		public:
			GLSwapchain(uint width, uint height);
			~GLSwapchain();

			bool Init(TextureDepth* depthTexture, api::RenderPass* vulkanRenderpass) override;
			bool Init(api::RenderPass* vulkanRenderpass) override { return true; };

			Texture* GetCurrentImage() override;
			uint32_t GetCurrentBufferId() const override;
			Framebuffer* GetFramebuffer(int id) const override;
			size_t GetSwapchainBufferCount() const override;
			uint GetFramebufferCount() const override { return static_cast<uint>(swapChainBuffers.size()); }
		private:
			std::vector<GLTexture2D*> swapChainBuffers;
			uint32_t currentBuffer = 0;
			std::vector<Framebuffer*> frameBuffers;

			uint m_Width;
			uint m_Height;
		};
	}
}
