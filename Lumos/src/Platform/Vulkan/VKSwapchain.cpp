#include "lmpch.h"
#include "VKSwapchain.h"
#include "VKTools.h"


namespace Lumos
{
	namespace Graphics
	{

		VKSwapchain::VKSwapchain(u32 width, u32 height)
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
            VkSurfaceCapabilitiesKHR surfaceCapabilities;
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VKDevice::Instance()->GetGPU(), VKDevice::Instance()->GetSurface(), &surfaceCapabilities);

			uint32_t numPresentModes;
			vkGetPhysicalDeviceSurfacePresentModesKHR(VKDevice::Instance()->GetGPU(), VKDevice::Instance()->GetSurface(), &numPresentModes, VK_NULL_HANDLE);

			VkPresentModeKHR * pPresentModes = new VkPresentModeKHR[numPresentModes];
			vkGetPhysicalDeviceSurfacePresentModesKHR(VKDevice::Instance()->GetGPU(), VKDevice::Instance()->GetSurface(), &numPresentModes, pPresentModes);

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
			swapChainCI.oldSwapchain = nullptr;
            swapChainCI.imageColorSpace = VKDevice::Instance()->GetColourSpace();
            swapChainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			swapChainCI.queueFamilyIndexCount = 0;
            swapChainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			swapChainCI.pQueueFamilyIndices = VK_NULL_HANDLE;
			swapChainCI.clipped = VK_TRUE;

            vkCreateSwapchainKHR(VKDevice::Instance()->GetDevice(), &swapChainCI, nullptr, &m_SwapChain);
    
			uint32_t swapChainImageCount;
            vkGetSwapchainImagesKHR(VKDevice::Instance()->GetDevice(), m_SwapChain, &swapChainImageCount, nullptr);

            VkImage * pSwapChainImages = lmnew VkImage[swapChainImageCount];
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
				VKTexture2D* swapChainBuffer = lmnew VKTexture2D(pSwapChainImages[i], imageView);

				m_SwapChainBuffers.push_back(swapChainBuffer);
			}

			delete[] pSwapChainImages;
			delete[] pPresentModes;

			return true;
		}

		void VKSwapchain::Unload()
		{
			for (uint32_t i = 0; i < m_SwapChainBuffers.size(); i++)
			{
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
        
        void VKSwapchain::MakeDefault()
        {
            CreateFunc = CreateFuncVulkan;
        }
        
		Swapchain* VKSwapchain::CreateFuncVulkan(u32 width, u32 height)
        {
            return lmnew VKSwapchain(width, height);
        }
	}
}
