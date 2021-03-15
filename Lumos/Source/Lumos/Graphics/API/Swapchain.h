#pragma once


namespace Lumos
{
	namespace Graphics
	{
		class Texture;
		class Framebuffer;
		class RenderPass;
		class CommandBuffer;

		class Swapchain
		{
		public:
			virtual ~Swapchain() = default;
			static Swapchain* Create(uint32_t width, uint32_t height);

			virtual bool Init(bool vsync) = 0;
			virtual Texture* GetCurrentImage() = 0;
			virtual Texture* GetImage(uint32_t id) = 0;
			virtual uint32_t GetCurrentBufferId() const = 0;
			virtual size_t GetSwapchainBufferCount() const = 0;
			virtual uint32_t GetFramebufferCount() const = 0;
			virtual Framebuffer* CreateFramebuffer(RenderPass* renderPass, uint32_t id) = 0;
            virtual CommandBuffer* GetCurrentCommandBuffer() const { return nullptr; }
			
        protected:
            static Swapchain* (*CreateFunc)(uint32_t, uint32_t);
		};
	}
}
