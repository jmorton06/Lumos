#pragma once
#include "LM.h"
#include "VKDevice.h"
#include "VKCommandBuffer.h"
#include "VKRenderpass.h"
#include "VKTexture.h"
#include "VKFramebuffer.h"
#include "Graphics/API/Swapchain.h"

namespace Lumos
{
	namespace Graphics
	{
		class VKSwapchain : public Swapchain
		{
		public:
			VKSwapchain(u32 width, u32 height);
			~VKSwapchain();

			bool Init() override;

			void Unload();

            vk::Result AcquireNextImage(vk::Semaphore signalSemaphore);
            void Present(vk::Semaphore waitSemaphore);

            vk::SwapchainKHR 	GetSwapchain() 				const { return m_SwapChain; }
			uint32_t 			GetCurrentBufferId() 		const override { return m_CurrentBuffer; };
			size_t 				GetSwapchainBufferCount() 	const override { return m_SwapChainBuffers.size(); };
			u32 		 		GetFramebufferCount() 		const override { return static_cast<u32>(m_SwapChainBuffers.size()); }
			VKTexture2D* 		GetTexture(int id) 			const { return m_SwapChainBuffers[id]; }
			Texture* 			GetCurrentImage() 			override { return (Texture*)m_SwapChainBuffers[m_CurrentBuffer]; };
			Texture* 			GetImage(u32 id) 			override { return (Texture*)m_SwapChainBuffers[id]; };
			Framebuffer*		CreateFramebuffer(RenderPass* renderPass, u32 id) override { return nullptr; };

            static void MakeDefault();
        protected:
            static Swapchain* CreateFuncVulkan(u32 width, u32 height);
            
		private:
            vk::SwapchainKHR 			m_SwapChain;
			std::vector<VKTexture2D*> 	m_SwapChainBuffers;
			uint32_t 					m_CurrentBuffer = 0;
			u32						m_Width;
			u32						m_Height;
		};
	}
}
