#pragma once
#include "Graphics/API/Swapchain.h"

namespace Lumos
{
	namespace Graphics
	{
		class GLTexture2D;

		class GLSwapchain : public Swapchain
		{
		public:
			GLSwapchain(u32 width, u32 height);
			~GLSwapchain();

			bool Init(bool vsync) override;

			Texture* GetCurrentImage() override;
			Texture* GetImage(u32 id) override { return nullptr; };
			uint32_t GetCurrentBufferId() const override;
			size_t GetSwapchainBufferCount() const override;
			u32 GetFramebufferCount() const override { return 1; }
			Framebuffer* CreateFramebuffer(RenderPass* renderPass, u32 id) override { return nullptr; }
            
            static void MakeDefault();
        protected:
            static Swapchain* CreateFuncGL(u32 width, u32 height);
            
		private:
			std::vector<GLTexture2D*> swapChainBuffers;
			uint32_t currentBuffer = 0;

			u32 m_Width;
			u32 m_Height;
		};
	}
}
