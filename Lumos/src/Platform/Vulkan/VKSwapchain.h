#pragma once
#include "LM.h"
#include "VKDevice.h"
#include "VKCommandBuffer.h"
#include "VKRenderpass.h"
#include "VKTexture2D.h"
#include "VKFramebuffer.h"
#include "Graphics/API/Swapchain.h"

namespace Lumos
{
	namespace graphics
	{
		class VKSwapchain : public api::Swapchain
		{
		public:
			VKSwapchain(uint width, uint height);
			~VKSwapchain();

			bool Init() override;

			void Unload();

            vk::Result AcquireNextImage(vk::Semaphore signalSemaphore);
            void Present(vk::Semaphore waitSemaphore);

            vk::SwapchainKHR 	GetSwapchain() 				const { return m_SwapChain; }
			uint32_t 			GetCurrentBufferId() 		const override { return m_CurrentBuffer; };
			size_t 				GetSwapchainBufferCount() 	const override { return m_SwapChainBuffers.size(); };
			uint 		 		GetFramebufferCount() 		const override { return static_cast<uint>(m_SwapChainBuffers.size()); }
			VKTexture2D* 		GetTexture(int id) 			const { return m_SwapChainBuffers[id]; }
			Texture* 			GetCurrentImage() 			override { return (Texture*)m_SwapChainBuffers[m_CurrentBuffer]; };
			Texture* 			GetImage(uint id) 			override { return (Texture*)m_SwapChainBuffers[id]; };
			Framebuffer*		CreateFramebuffer(api::RenderPass* renderPass, uint id) override { return nullptr; };

		private:
            vk::SwapchainKHR 			m_SwapChain;
			std::vector<VKTexture2D*> 	m_SwapChainBuffers;
			uint32_t 					m_CurrentBuffer = 0;
			uint						m_Width;
			uint						m_Height;
		};
	}
}
