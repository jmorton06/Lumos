#pragma once
#include "LM.h"

namespace Lumos
{
	namespace Graphics
	{
		class Texture;
		class Framebuffer;
		class RenderPass;

		class Swapchain
		{
		public:
			virtual ~Swapchain() = default;
			static Swapchain* Create(u32 width, u32 height);

			virtual bool Init() = 0;
			virtual Texture* GetCurrentImage() = 0;
			virtual Texture* GetImage(u32 id) = 0;
			virtual uint32_t GetCurrentBufferId() const = 0;
			virtual size_t GetSwapchainBufferCount() const = 0;
			virtual u32 GetFramebufferCount() const = 0;
			virtual Framebuffer* CreateFramebuffer(RenderPass* renderPass, u32 id) = 0;
            
        protected:
            static Swapchain* (*CreateFunc)(u32, u32);
		};
	}
}
