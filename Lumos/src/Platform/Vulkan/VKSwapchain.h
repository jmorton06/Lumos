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
			VKSwapchain(u32 width, u32 height);
			~VKSwapchain();

			bool Init(bool vsync) override;
            void Init(bool vsync, Window* windowHandle);
            VkResult AcquireNextImage(VkSemaphore signalSemaphore);
            void Present(VkSemaphore waitSemaphore);

            VkSurfaceKHR GetSurface() const { return m_Surface; }
            VkSwapchainKHR GetSwapchain() const { return m_SwapChain; }
			uint32_t GetCurrentBufferId() const override { return m_CurrentBuffer; };
			size_t GetSwapchainBufferCount() const override { return m_SwapChainBuffers.size(); };
			u32 GetFramebufferCount() const override { return static_cast<u32>(m_SwapChainBuffers.size()); }
            Texture2D* GetTexture(int id) const { return m_SwapChainBuffers[id]; }
			Texture* GetCurrentImage() override { return (Texture*)m_SwapChainBuffers[m_CurrentBuffer]; };
			Texture* GetImage(u32 id) override { return (Texture*)m_SwapChainBuffers[id]; };
			Framebuffer* CreateFramebuffer(RenderPass* renderPass, u32 id) override { return nullptr; };
			
			VkSurfaceKHR CreatePlatformSurface(VkInstance vkInstance, Window* window);

            static void MakeDefault();
        protected:
            static Swapchain* CreateFuncVulkan(u32 width, u32 height);
            
			private:
			
			void FindImageFormatAndColorSpace();
			
            VkSwapchainKHR m_SwapChain;
			std::vector<Texture2D*> m_SwapChainBuffers;
			uint32_t m_CurrentBuffer = 0;
			u32	m_Width;
			u32	m_Height;
			uint32_t m_QueueNodeIndex = UINT32_MAX;
			
            VkSurfaceKHR m_Surface;
			VkFormat m_ColorFormat;
			VkColorSpaceKHR m_ColorSpace;
		};
	}
}
