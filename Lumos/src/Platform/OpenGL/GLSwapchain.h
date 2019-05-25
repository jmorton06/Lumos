#pragma once
#include "LM.h"
#include "Graphics/API/Swapchain.h"

namespace lumos
{
	namespace graphics
	{
		class GLTexture2D;

		class GLSwapchain : public Swapchain
		{
		public:
			GLSwapchain(uint width, uint height);
			~GLSwapchain();

			bool Init() override;

			Texture* GetCurrentImage() override;
			Texture* GetImage(uint id) override { return nullptr; };
			uint32_t GetCurrentBufferId() const override;
			size_t GetSwapchainBufferCount() const override;
			uint GetFramebufferCount() const override { return 1; }
			Framebuffer* CreateFramebuffer(RenderPass* renderPass, uint id) override { return nullptr; }
		private:
			std::vector<GLTexture2D*> swapChainBuffers;
			uint32_t currentBuffer = 0;

			uint m_Width;
			uint m_Height;
		};
	}
}
