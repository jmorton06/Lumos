#include "Precompiled.h"
#include "VKDevice.h"
#include "VKSwapchain.h"
#include "VKTools.h"
#include "Core/Application.h"

namespace Lumos
{
	namespace Graphics
	{

		VKSwapchain::VKSwapchain(u32 width, u32 height)
		{
			m_Width = width;
			m_Height = height;
			m_SwapChain = VK_NULL_HANDLE;
		}

		VKSwapchain::~VKSwapchain()
		{
			for (uint32_t i = 0; i < m_SwapChainBuffers.size(); i++)
			{
				delete m_SwapChainBuffers[i];
			}
			vkDestroySwapchainKHR(VKDevice::Get().GetDevice(), m_SwapChain, VK_NULL_HANDLE);
		}
    
        void VKSwapchain::Init(bool vsync, Window* windowHandle)
        {
            m_Surface = CreatePlatformSurface(VKContext::Get()->GetVKInstance(), windowHandle);
            Init(vsync);
        }

		bool VKSwapchain::Init(bool vsync)
		{
            FindImageFormatAndColorSpace();
            
            if(!m_Surface)
            {
                Debug::Log::Critical("[VULKAN] Failed to create window surface!");
            }

			// Swap chain
            VkSurfaceCapabilitiesKHR surfaceCapabilities;
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VKDevice::Get().GetGPU(), m_Surface, &surfaceCapabilities);

			uint32_t numPresentModes;
			vkGetPhysicalDeviceSurfacePresentModesKHR(VKDevice::Get().GetGPU(), m_Surface, &numPresentModes, VK_NULL_HANDLE);

			VkPresentModeKHR * pPresentModes = new VkPresentModeKHR[numPresentModes];
			vkGetPhysicalDeviceSurfacePresentModesKHR(VKDevice::Get().GetGPU(), m_Surface, &numPresentModes, pPresentModes);

            VkExtent2D swapChainExtent;

			swapChainExtent.width = static_cast<uint32_t>(m_Width);
			swapChainExtent.height = static_cast<uint32_t>(m_Height);

			VkPresentModeKHR swapChainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
			for (uint32_t i = 0; i < numPresentModes; i++)
			{
				if (pPresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
					swapChainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
				if ((swapChainPresentMode != VK_PRESENT_MODE_MAILBOX_KHR) && (pPresentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR))
					swapChainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
			}

			// Use triple-buffering
			uint32_t numSwapChainImages = surfaceCapabilities.maxImageCount;
			if (numSwapChainImages > 3)
				numSwapChainImages = 3;

            VkSurfaceTransformFlagBitsKHR preTransform;
			if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
				preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
			else
				preTransform = surfaceCapabilities.currentTransform;

            VkSwapchainCreateInfoKHR swapChainCI{};
			swapChainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			swapChainCI.surface = m_Surface;
			swapChainCI.minImageCount = numSwapChainImages;
			swapChainCI.imageFormat = m_ColorFormat;
			swapChainCI.imageExtent.width = swapChainExtent.width;
			swapChainCI.imageExtent.height = swapChainExtent.height;
			swapChainCI.preTransform = preTransform;
            swapChainCI.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
			swapChainCI.imageArrayLayers = 1;
			swapChainCI.presentMode = swapChainPresentMode;
			swapChainCI.oldSwapchain = VK_NULL_HANDLE;
            swapChainCI.imageColorSpace = m_ColorSpace;
            swapChainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			swapChainCI.queueFamilyIndexCount = 0;
            swapChainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			swapChainCI.pQueueFamilyIndices = VK_NULL_HANDLE;
			swapChainCI.clipped = VK_TRUE;

            VK_CHECK_RESULT(vkCreateSwapchainKHR(VKDevice::Get().GetDevice(), &swapChainCI, VK_NULL_HANDLE, &m_SwapChain));

			uint32_t swapChainImageCount;
            VK_CHECK_RESULT(vkGetSwapchainImagesKHR(VKDevice::Get().GetDevice(), m_SwapChain, &swapChainImageCount, VK_NULL_HANDLE));

            VkImage * pSwapChainImages = new VkImage[swapChainImageCount];
            VK_CHECK_RESULT(vkGetSwapchainImagesKHR(VKDevice::Get().GetDevice(), m_SwapChain, &swapChainImageCount, pSwapChainImages));

			for (uint32_t i = 0; i < swapChainImageCount; i++)
			{
                VkImageViewCreateInfo viewCI{};
				viewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				viewCI.format = m_ColorFormat;
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
				VK_CHECK_RESULT(vkCreateImageView(VKDevice::Get().GetDevice(), &viewCI, VK_NULL_HANDLE, &imageView));
				VKTexture2D* swapChainBuffer = new VKTexture2D(pSwapChainImages[i], imageView);

				m_SwapChainBuffers.push_back(swapChainBuffer);
			}

			delete[] pSwapChainImages;
			delete[] pPresentModes;
			
			return true;
		}

        VkResult VKSwapchain::AcquireNextImage(VkSemaphore signalSemaphore)
		{
			return vkAcquireNextImageKHR(VKDevice::Get().GetDevice(), m_SwapChain, UINT64_MAX, signalSemaphore, VK_NULL_HANDLE, &m_CurrentBuffer);
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
			VK_CHECK_RESULT(vkQueuePresentKHR(VKDevice::Get().GetPresentQueue(), &present));
		}

        void VKSwapchain::MakeDefault()
        {
            CreateFunc = CreateFuncVulkan;
        }

		Swapchain* VKSwapchain::CreateFuncVulkan(u32 width, u32 height)
        {
            return new VKSwapchain(width, height);
        }
	}
	
    void Graphics::VKSwapchain::FindImageFormatAndColorSpace()
	{
		VkPhysicalDevice physicalDevice = VKDevice::Get().GetPhysicalDevice()->GetVulkanPhysicalDevice();
		
		// Get list of supported surface formats
		uint32_t formatCount;
		VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_Surface, &formatCount, NULL));
		LUMOS_ASSERT(formatCount > 0, "");
		
		std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
		VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_Surface, &formatCount, surfaceFormats.data()));
		
		// If the surface format list only includes one entry with VK_FORMAT_UNDEFINED,
		// there is no preferered format, so we assume VK_FORMAT_B8G8R8A8_UNORM
		if ((formatCount == 1) && (surfaceFormats[0].format == VK_FORMAT_UNDEFINED))
		{
			m_ColorFormat = VK_FORMAT_B8G8R8A8_UNORM;
			m_ColorSpace = surfaceFormats[0].colorSpace;
		}
		else
		{
			// iterate over the list of available surface format and
			// check for the presence of VK_FORMAT_B8G8R8A8_UNORM
			bool found_B8G8R8A8_UNORM = false;
			for (auto&& surfaceFormat : surfaceFormats)
			{
                if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM)
				{
					m_ColorFormat = surfaceFormat.format;
					m_ColorSpace = surfaceFormat.colorSpace;
					found_B8G8R8A8_UNORM = true;
					break;
				}
			}
			
			// in case VK_FORMAT_B8G8R8A8_UNORM is not available
			// select the first available color format
			if (!found_B8G8R8A8_UNORM)
			{
				m_ColorFormat = surfaceFormats[0].format;
				m_ColorSpace = surfaceFormats[0].colorSpace;
			}
		}
		
	}
}
