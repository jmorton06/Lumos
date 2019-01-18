#pragma once
#include "LM.h"

namespace Lumos
{
	class TextureDepth;
	class Texture;
	class Framebuffer;

	namespace graphics
	{
		namespace api
		{
			class RenderPass;
			class Swapchain
			{
			public:
				virtual ~Swapchain() = default;
				static Swapchain* Create(uint width, uint height);

				virtual bool Init() = 0;
				virtual Texture* GetCurrentImage() = 0;
				virtual Texture* GetImage(uint id) = 0;
				virtual uint32_t GetCurrentBufferId() const = 0;
				virtual size_t GetSwapchainBufferCount() const = 0;
				virtual uint GetFramebufferCount() const = 0;
				virtual Framebuffer* CreateFramebuffer(RenderPass* renderPass, uint id) = 0;
			};
		}
	}
}