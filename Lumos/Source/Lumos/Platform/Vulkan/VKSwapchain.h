#pragma once
#include "VKCommandBuffer.h"
#include "VKRenderpass.h"
#include "VKFramebuffer.h"
#include "Graphics/API/Swapchain.h"

namespace Lumos
{
    class Window;
	namespace Graphics
	{
        class Texture2D;
		class VKSwapchain : public Swapchain
		{
		public:
			VKSwapchain(uint32_t width, uint32_t height);
			~VKSwapchain();

			bool Init(bool vsync) override;
            void Init(bool vsync, Window* windowHandle);
            VkResult AcquireNextImage(VkSemaphore signalSemaphore);
            void Present(VkSemaphore waitSemaphore);

            VkSurfaceKHR GetSurface() const { return m_Surface; }
            VkSwapchainKHR GetSwapchain() const { return m_SwapChain; }
			uint32_t GetCurrentBufferId() const override { return m_CurrentBuffer; };
			size_t GetSwapchainBufferCount() const override { return m_SwapChainBuffers.size(); };
			uint32_t GetFramebufferCount() const override { return static_cast<uint32_t>(m_SwapChainBuffers.size()); }
            Texture2D* GetTexture(int id) const { return m_SwapChainBuffers[id]; }
			Texture* GetCurrentImage() override { return (Texture*)m_SwapChainBuffers[m_CurrentBuffer]; };
			Texture* GetImage(uint32_t id) override { return (Texture*)m_SwapChainBuffers[id]; };
			Framebuffer* CreateFramebuffer(RenderPass* renderPass, uint32_t id) override { return nullptr; };
			
			VkSurfaceKHR CreatePlatformSurface(VkInstance vkInstance, Window* window);
            VkSemaphore GetImageAcquiredSemaphore();
			CommandBuffer* GetCurrentCommandBuffer() const override;

            static void MakeDefault();
        protected:
            static Swapchain* CreateFuncVulkan(uint32_t width, uint32_t height);
            
			private:
			
			void FindImageFormatAndColourSpace();
			
            VkSwapchainKHR m_SwapChain;
			std::vector<Texture2D*> m_SwapChainBuffers;
            std::vector<VkSemaphore> m_ImageAquiredSemaphores;
			std::vector<CommandBuffer*> m_CommandBuffers;
			
			uint32_t m_CurrentBuffer = 0;
			uint32_t m_AcquireImageIndex = 0;
			uint32_t m_Width;
			uint32_t m_Height;
			uint32_t m_QueueNodeIndex = UINT32_MAX;
			
            VkSurfaceKHR m_Surface;
			VkFormat m_ColourFormat;
			VkColorSpaceKHR m_ColourSpace;
		};
	}
}
