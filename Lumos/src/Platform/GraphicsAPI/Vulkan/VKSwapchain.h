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

			bool Init(TextureDepth* depthImageView, api::RenderPass* vulkanRenderpass) override;
			bool Init(api::RenderPass* vulkanRenderpass) override;

			void Unload();

			VkResult AcquireNextImage(VkSemaphore signalSemaphore);
			void Present(VkSemaphore waitSemaphore);

			VkSwapchainKHR 		GetSwapchain() 				const { return m_SwapChain; }
			api::RenderPass* 	GetRenderPass() 			const { return m_RenderPass; }
			uint32_t 			GetCurrentBufferId() 		const override { return m_CurrentBuffer; };
			Framebuffer* 		GetFramebuffer(int id) 		const override { return reinterpret_cast<Framebuffer*>(m_FrameBuffers[id]); };
			size_t 				GetSwapchainBufferCount() 	const override { return m_SwapChainBuffers.size(); };
			uint 		 		GetFramebufferCount() 		const override { return static_cast<uint>(m_SwapChainBuffers.size()); }
			VKTexture2D* 		GetTexture(int id) 			const { return m_SwapChainBuffers[id]; }
			Texture* 			GetCurrentImage() 			override { return (Texture*)m_SwapChainBuffers[m_CurrentBuffer]; };

		private:
			VkSwapchainKHR 				m_SwapChain;
			std::vector<VKTexture2D*> 	m_SwapChainBuffers;
			uint32_t 					m_CurrentBuffer = 0;
			std::vector<VKFrameBuffer*> m_FrameBuffers;
			api::RenderPass* 			m_RenderPass;
			uint						m_Width;
			uint						m_Height;
		};
	}
}
