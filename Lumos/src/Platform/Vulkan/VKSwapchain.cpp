#include "LM.h"
#include "VKSwapchain.h"
#include "VKTools.h"


namespace Lumos
{
	namespace Graphics
	{

		VKSwapchain::VKSwapchain(uint width, uint height)
		{
			m_Width = width;
			m_Height = height;
		}

		VKSwapchain::~VKSwapchain()
		{
			Unload();
		}

		bool VKSwapchain::Init()
		{
			// Swap chain
            vk::SurfaceCapabilitiesKHR surfaceCapabilities;
            VKDevice::Instance()->GetGPU().getSurfaceCapabilitiesKHR( VKDevice::Instance()->GetSurface(), &surfaceCapabilities);

			uint32_t numPresentModes;
			VKDevice::Instance()->GetGPU().getSurfacePresentModesKHR(VKDevice::Instance()->GetSurface(), &numPresentModes, VK_NULL_HANDLE);

            vk::PresentModeKHR * pPresentModes = new vk::PresentModeKHR[numPresentModes];
            VKDevice::Instance()->GetGPU().getSurfacePresentModesKHR(VKDevice::Instance()->GetSurface(), &numPresentModes, pPresentModes);

            vk::Extent2D swapChainExtent;

			swapChainExtent.width = static_cast<uint32_t>(m_Width);
			swapChainExtent.height = static_cast<uint32_t>(m_Height);

            vk::PresentModeKHR swapChainPresentMode = vk::PresentModeKHR::eFifo;
			for (uint32_t i = 0; i < numPresentModes; i++)
			{
                if (pPresentModes[i] == vk::PresentModeKHR::eMailbox)
					swapChainPresentMode = vk::PresentModeKHR::eMailbox;
                if ((swapChainPresentMode != vk::PresentModeKHR::eMailbox) && (pPresentModes[i] == vk::PresentModeKHR::eImmediate))
					swapChainPresentMode = vk::PresentModeKHR::eImmediate;
			}

			// Use double-buffering
			uint32_t numSwapChainImages = surfaceCapabilities.minImageCount;
			if (numSwapChainImages > surfaceCapabilities.maxImageCount)
				numSwapChainImages = surfaceCapabilities.maxImageCount;

            vk::SurfaceTransformFlagBitsKHR preTransform;
            if (surfaceCapabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity)
				preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
			else
				preTransform = surfaceCapabilities.currentTransform;

            vk::SwapchainCreateInfoKHR swapChainCI{};
			swapChainCI.surface = VKDevice::Instance()->GetSurface();
			swapChainCI.minImageCount = numSwapChainImages;
			swapChainCI.imageFormat = VKDevice::Instance()->GetFormat();
			swapChainCI.imageExtent.width = swapChainExtent.width;
			swapChainCI.imageExtent.height = swapChainExtent.height;
			swapChainCI.preTransform = preTransform;
            swapChainCI.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
			swapChainCI.imageArrayLayers = 1;
			swapChainCI.presentMode = swapChainPresentMode;
			swapChainCI.oldSwapchain = nullptr;
            swapChainCI.imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
            swapChainCI.imageSharingMode = vk::SharingMode::eExclusive;
			swapChainCI.queueFamilyIndexCount = 0;
            swapChainCI.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
			swapChainCI.pQueueFamilyIndices = VK_NULL_HANDLE;
			swapChainCI.clipped = VK_TRUE;

            m_SwapChain = VKDevice::Instance()->GetDevice().createSwapchainKHR(swapChainCI);
    
			uint32_t swapChainImageCount;
            VKDevice::Instance()->GetDevice().getSwapchainImagesKHR(m_SwapChain, &swapChainImageCount, nullptr);

            vk::Image * pSwapChainImages = new vk::Image[swapChainImageCount];
            VKDevice::Instance()->GetDevice().getSwapchainImagesKHR(m_SwapChain, &swapChainImageCount, pSwapChainImages);

			for (uint32_t i = 0; i < swapChainImageCount; i++)
			{
                vk::ImageViewCreateInfo viewCI{};
				viewCI.format = VKDevice::Instance()->GetFormat();
                viewCI.components.r = vk::ComponentSwizzle::eR;
				viewCI.components.g = vk::ComponentSwizzle::eG;
				viewCI.components.b = vk::ComponentSwizzle::eB;
				viewCI.components.a = vk::ComponentSwizzle::eA;
                viewCI.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
				viewCI.subresourceRange.baseMipLevel = 0;
				viewCI.subresourceRange.levelCount = 1;
				viewCI.subresourceRange.baseArrayLayer = 0;
				viewCI.subresourceRange.layerCount = 1;
                viewCI.viewType = vk::ImageViewType::e2D;
				viewCI.image = pSwapChainImages[i];

                vk::ImageView imageView = VKDevice::Instance()->GetDevice().createImageView(viewCI);
				VKTexture2D* swapChainBuffer = new VKTexture2D(pSwapChainImages[i], imageView);

				m_SwapChainBuffers.push_back(swapChainBuffer);
			}

			delete[] pSwapChainImages;
			delete[] pPresentModes;

			Graphics::VKDevice::Instance()->m_SwapChainSize = static_cast<uint>(GetSwapchainBufferCount());

			return true;
		}

		void VKSwapchain::Unload()
		{
			for (uint32_t i = 0; i < m_SwapChainBuffers.size(); i++)
			{
				delete m_SwapChainBuffers[i];
			}
            VKDevice::Instance()->GetDevice().destroySwapchainKHR(m_SwapChain);
		}

        vk::Result VKSwapchain::AcquireNextImage(vk::Semaphore signalSemaphore)
		{
            return VKDevice::Instance()->GetDevice().acquireNextImageKHR(m_SwapChain, UINT64_MAX, signalSemaphore, nullptr, &m_CurrentBuffer);
		}

        void VKSwapchain::Present(vk::Semaphore waitSemaphore)
		{
            vk::PresentInfoKHR present;
			present.pNext = VK_NULL_HANDLE;
			present.swapchainCount = 1;
			present.pSwapchains = &m_SwapChain;
			present.pImageIndices = &m_CurrentBuffer;
			present.waitSemaphoreCount = 1;
			present.pWaitSemaphores = &waitSemaphore;
			present.pResults = VK_NULL_HANDLE;
            VKDevice::Instance()->GetPresentQueue().presentKHR(present);
		}
	}
}
