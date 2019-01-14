#include "JM.h"
#include "VKSwapchain.h"
#include "VKTools.h"


namespace jm
{
	namespace graphics
	{

		VKSwapchain::VKSwapchain(uint width, uint height)
		{
			m_SwapChain = VK_NULL_HANDLE;
			m_Width = width;
			m_Height = height;
		}

		VKSwapchain::~VKSwapchain()
		{
			Unload();
			m_SwapChain = VK_NULL_HANDLE;
		}

		bool VKSwapchain::Init(TextureDepth* depthImageView, api::RenderPass* vulkanRenderpass)
		{
			m_RenderPass = vulkanRenderpass;
			// Swap chain
			VkSurfaceCapabilitiesKHR surfaceCapabilities;

			VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VKDevice::Instance()->GetGPU(), VKDevice::Instance()->GetSurface(), &surfaceCapabilities));

			uint32_t numPresentModes;
			VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(VKDevice::Instance()->GetGPU(), VKDevice::Instance()->GetSurface(), &numPresentModes, VK_NULL_HANDLE));

			VkPresentModeKHR * pPresentModes = new VkPresentModeKHR[numPresentModes];
			VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(VKDevice::Instance()->GetGPU(), VKDevice::Instance()->GetSurface(), &numPresentModes, pPresentModes));

			VkExtent2D swapChainExtent;

			swapChainExtent.width = static_cast<uint32_t>(m_Width);
			swapChainExtent.height = static_cast<uint32_t>(m_Height);

			VkPresentModeKHR swapChainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
			for (uint32_t i = 0; i < numPresentModes; i++)
			{
				if (pPresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
					swapChainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
				if ((swapChainPresentMode != VK_PRESENT_MODE_MAILBOX_KHR) && (pPresentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR))
					swapChainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
			}

			// Use double-buffering
			uint32_t numSwapChainImages = surfaceCapabilities.minImageCount;
			if (numSwapChainImages > surfaceCapabilities.maxImageCount)
				numSwapChainImages = surfaceCapabilities.maxImageCount;

			VkSurfaceTransformFlagBitsKHR preTransform;
			if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
				preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
			else
				preTransform = surfaceCapabilities.currentTransform;

			VkSwapchainCreateInfoKHR swapChainCI{};
			swapChainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			swapChainCI.surface = VKDevice::Instance()->GetSurface();
			swapChainCI.minImageCount = numSwapChainImages;
			swapChainCI.imageFormat = VKDevice::Instance()->GetFormat();
			swapChainCI.imageExtent.width = swapChainExtent.width;
			swapChainCI.imageExtent.height = swapChainExtent.height;
			swapChainCI.preTransform = preTransform;
			swapChainCI.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
			swapChainCI.imageArrayLayers = 1;
			swapChainCI.presentMode = swapChainPresentMode;
			swapChainCI.oldSwapchain = VK_NULL_HANDLE;
			swapChainCI.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
			swapChainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			swapChainCI.queueFamilyIndexCount = 0;
			swapChainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			swapChainCI.pQueueFamilyIndices = VK_NULL_HANDLE;
			swapChainCI.clipped = VK_TRUE;

			VK_CHECK_RESULT(vkCreateSwapchainKHR(VKDevice::Instance()->GetDevice(), &swapChainCI, VK_NULL_HANDLE, &m_SwapChain));

			uint32_t swapChainImageCount;
			VK_CHECK_RESULT(vkGetSwapchainImagesKHR(VKDevice::Instance()->GetDevice(), m_SwapChain, &swapChainImageCount, VK_NULL_HANDLE));

			VkImage * pSwapChainImages = new VkImage[swapChainImageCount];
			vkGetSwapchainImagesKHR(VKDevice::Instance()->GetDevice(), m_SwapChain, &swapChainImageCount, pSwapChainImages);

			for (uint32_t i = 0; i < swapChainImageCount; i++)
			{
				VkImageViewCreateInfo viewCI{};
				viewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				viewCI.format = VKDevice::Instance()->GetFormat();
				viewCI.components.r = VK_COMPONENT_SWIZZLE_R;
				viewCI.components.g = VK_COMPONENT_SWIZZLE_G;
				viewCI.components.b = VK_COMPONENT_SWIZZLE_B;
				viewCI.components.a = VK_COMPONENT_SWIZZLE_A;
				viewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				viewCI.subresourceRange.baseMipLevel = 0;
				viewCI.subresourceRange.levelCount = 1;
				viewCI.subresourceRange.baseArrayLayer = 0;
				viewCI.subresourceRange.layerCount = 1;
				viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
				viewCI.flags = 0;
				viewCI.image = pSwapChainImages[i];

				VkImageView imageView;
				vkCreateImageView(VKDevice::Instance()->GetDevice(), &viewCI, VK_NULL_HANDLE, &imageView);
				VKTexture2D* swapChainBuffer = new VKTexture2D(pSwapChainImages[i], imageView);

				m_SwapChainBuffers.push_back(swapChainBuffer);
			}

			delete[] pSwapChainImages;
			delete[] pPresentModes;

			TextureType attachmentTypes[2];
			attachmentTypes[0] = TextureType::COLOUR;
			attachmentTypes[1] = TextureType::DEPTH;

			Texture* attachments[2];
			attachments[1] = reinterpret_cast<Texture*>(depthImageView);//new VKTexture2D(VkImage(), depthImageView);
			FrameBufferInfo bufferInfo{};
			bufferInfo.width = m_Width;
			bufferInfo.height = m_Height;
			bufferInfo.attachmentCount = 2;
			bufferInfo.renderPass = reinterpret_cast<api::RenderPass*>(vulkanRenderpass);
			bufferInfo.attachmentTypes = attachmentTypes;

			for (uint32_t i = 0; i < swapChainImageCount; i++)
			{
				attachments[0] = m_SwapChainBuffers[i];
				bufferInfo.attachments = attachments;

				m_FrameBuffers.emplace_back(new VKFrameBuffer(bufferInfo));
			}

			graphics::VKDevice::Instance()->m_SwapChainSize = static_cast<uint>(GetSwapchainBufferCount());

			return true;
		}

		bool VKSwapchain::Init(api::RenderPass* vulkanRenderpass)
		{
			m_RenderPass = vulkanRenderpass; graphics::VKDevice::Instance()->m_SwapChainSize = static_cast<uint>(GetSwapchainBufferCount()); return true;
		};


		void VKSwapchain::Unload()
		{
			for (uint32_t i = 0; i < m_SwapChainBuffers.size(); i++)
			{
				delete m_FrameBuffers[i];
				delete m_SwapChainBuffers[i];
			}
			vkDestroySwapchainKHR(VKDevice::Instance()->GetDevice(), m_SwapChain, VK_NULL_HANDLE);
		}

		VkResult VKSwapchain::AcquireNextImage(VkSemaphore signalSemaphore)
		{
			return vkAcquireNextImageKHR(VKDevice::Instance()->GetDevice(), m_SwapChain, UINT64_MAX, signalSemaphore, VK_NULL_HANDLE, &m_CurrentBuffer);
		}

		void VKSwapchain::Present(VkSemaphore waitSemaphore)
		{
			VkPresentInfoKHR present;
			present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			present.pNext = VK_NULL_HANDLE;
			present.swapchainCount = 1;
			present.pSwapchains = &m_SwapChain;
			present.pImageIndices = &m_CurrentBuffer;
			present.waitSemaphoreCount = 1;
			present.pWaitSemaphores = &waitSemaphore;
			present.pResults = VK_NULL_HANDLE;
			vkQueuePresentKHR(VKDevice::Instance()->GetPresentQueue(), &present);
		}
	}
}
