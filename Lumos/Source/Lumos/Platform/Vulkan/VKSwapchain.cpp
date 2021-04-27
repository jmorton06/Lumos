#include "Precompiled.h"
#include "VKDevice.h"
#include "VKSwapchain.h"
#include "VKTools.h"
#include "VKFence.h"
#include "Core/Application.h"

namespace Lumos
{
    namespace Graphics
    {
        VKSwapchain::VKSwapchain(uint32_t width, uint32_t height)
        {
            m_Width = width;
            m_Height = height;
            m_SwapChain = VK_NULL_HANDLE;
        }

        VKSwapchain::~VKSwapchain()
        {
            for(uint32_t i = 0; i < m_SwapChainBuffers.size(); i++)
            {
                //TODO: is this needed?
                //m_Frames[i].RenderFence->Wait();

                vkDestroyCommandPool(VKDevice::Get().GetDevice(), m_Frames[i].CommandPool, nullptr);
                vkDestroySemaphore(VKDevice::Get().GetDevice(), m_Frames[i].PresentSemaphore, nullptr);
                vkDestroySemaphore(VKDevice::Get().GetDevice(), m_Frames[i].RenderSemaphore, nullptr);

                delete m_Frames[i].RenderFence;
                delete m_SwapChainBuffers[i];
            }
            if(m_Surface != VK_NULL_HANDLE)
            {
                vkDestroySurfaceKHR(VKContext::Get()->GetVKInstance(), m_Surface, nullptr);
            }
            vkDestroySwapchainKHR(VKDevice::Get().GetDevice(), m_SwapChain, VK_NULL_HANDLE);
        }

        void VKSwapchain::Init(bool vsync, Window* windowHandle)
        {
            LUMOS_PROFILE_FUNCTION();
            m_Surface = CreatePlatformSurface(VKContext::Get()->GetVKInstance(), windowHandle);
            Init(vsync);
        }

        FrameData& VKSwapchain::GetCurrentFrameData()
        {
            LUMOS_ASSERT(m_CurrentBuffer < m_SwapChainBuffers.size(), "Incorrect swapchain buffer index");
            return m_Frames[m_CurrentBuffer];
        }

        bool IsPresentModeSupported(const std::vector<VkPresentModeKHR>& supportedModes, VkPresentModeKHR presentMode)
        {
            for(const auto& mode : supportedModes)
            {
                if(mode == presentMode)
                {
                    return true;
                }
            }
            return false;
        }

        VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& supportedModes, bool vsync)
        {
            VkPresentModeKHR presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
            if(vsync)
            {
                if(IsPresentModeSupported(supportedModes, VK_PRESENT_MODE_MAILBOX_KHR))
                {
                    presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                }
                else if(IsPresentModeSupported(supportedModes, VK_PRESENT_MODE_FIFO_KHR))
                {
                    presentMode = VK_PRESENT_MODE_FIFO_KHR;
                }
                else if(IsPresentModeSupported(supportedModes, VK_PRESENT_MODE_IMMEDIATE_KHR))
                {
                    presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
                }
                else
                {
                    LUMOS_LOG_ERROR("Failed to find supported presentation mode.");
                }
            }
            else
            {
                if(IsPresentModeSupported(supportedModes, VK_PRESENT_MODE_IMMEDIATE_KHR))
                {
                    presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
                }
                else if(IsPresentModeSupported(supportedModes, VK_PRESENT_MODE_FIFO_RELAXED_KHR))
                {
                    presentMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
                }
                else if(IsPresentModeSupported(supportedModes, VK_PRESENT_MODE_FIFO_KHR))
                {
                    presentMode = VK_PRESENT_MODE_FIFO_KHR;
                }
                else
                {
                    LUMOS_LOG_ERROR("Failed to find supported presentation mode.");
                }
            }

            return presentMode;
        }

        bool VKSwapchain::Init(bool vsync)
        {
            LUMOS_PROFILE_FUNCTION();
            FindImageFormatAndColourSpace();

            if(!m_Surface)
            {
                LUMOS_LOG_CRITICAL("[VULKAN] Failed to create window surface!");
            }

            // Swap chain
            VkSurfaceCapabilitiesKHR surfaceCapabilities;
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VKDevice::Get().GetGPU(), m_Surface, &surfaceCapabilities);

            uint32_t numPresentModes;
            vkGetPhysicalDeviceSurfacePresentModesKHR(VKDevice::Get().GetGPU(), m_Surface, &numPresentModes, VK_NULL_HANDLE);

            std::vector<VkPresentModeKHR> pPresentModes(numPresentModes);
            vkGetPhysicalDeviceSurfacePresentModesKHR(VKDevice::Get().GetGPU(), m_Surface, &numPresentModes, pPresentModes.data());

            VkExtent2D swapChainExtent;

            swapChainExtent.width = static_cast<uint32_t>(m_Width);
            swapChainExtent.height = static_cast<uint32_t>(m_Height);

            VkPresentModeKHR swapChainPresentMode = ChoosePresentMode(pPresentModes, vsync);

            // Use triple-buffering
            uint32_t numSwapChainImages = surfaceCapabilities.maxImageCount;
            if(numSwapChainImages > 3)
                numSwapChainImages = 3;

            VkSurfaceTransformFlagBitsKHR preTransform;
            if(surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
                preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
            else
                preTransform = surfaceCapabilities.currentTransform;

            VkSwapchainCreateInfoKHR swapChainCI {};
            swapChainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            swapChainCI.surface = m_Surface;
            swapChainCI.minImageCount = numSwapChainImages;
            swapChainCI.imageFormat = m_ColourFormat;
            swapChainCI.imageExtent.width = swapChainExtent.width;
            swapChainCI.imageExtent.height = swapChainExtent.height;
            swapChainCI.preTransform = preTransform;
            swapChainCI.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
            swapChainCI.imageArrayLayers = 1;
            swapChainCI.presentMode = swapChainPresentMode;
            swapChainCI.oldSwapchain = VK_NULL_HANDLE;
            swapChainCI.imageColorSpace = m_ColourSpace;
            swapChainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            swapChainCI.queueFamilyIndexCount = 0;
            swapChainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            swapChainCI.pQueueFamilyIndices = VK_NULL_HANDLE;
            swapChainCI.clipped = VK_TRUE;

            VK_CHECK_RESULT(vkCreateSwapchainKHR(VKDevice::Get().GetDevice(), &swapChainCI, VK_NULL_HANDLE, &m_SwapChain));

            uint32_t swapChainImageCount;
            VK_CHECK_RESULT(vkGetSwapchainImagesKHR(VKDevice::Get().GetDevice(), m_SwapChain, &swapChainImageCount, VK_NULL_HANDLE));

            VkImage* pSwapChainImages = new VkImage[swapChainImageCount];
            VK_CHECK_RESULT(vkGetSwapchainImagesKHR(VKDevice::Get().GetDevice(), m_SwapChain, &swapChainImageCount, pSwapChainImages));

            for(uint32_t i = 0; i < swapChainImageCount; i++)
            {
                VkImageViewCreateInfo viewCI {};
                viewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                viewCI.format = m_ColourFormat;
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

                VkSemaphoreCreateInfo semaphoreInfo = {};
                semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
                semaphoreInfo.pNext = nullptr;

                m_Frames[i].RenderFence = new VKFence();

                VK_CHECK_RESULT(vkCreateSemaphore(VKDevice::Get().GetDevice(), &semaphoreInfo, nullptr, &m_Frames[i].PresentSemaphore));
                VK_CHECK_RESULT(vkCreateSemaphore(VKDevice::Get().GetDevice(), &semaphoreInfo, nullptr, &m_Frames[i].RenderSemaphore));

                VkCommandPoolCreateInfo cmdPoolCI {};
                cmdPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
                cmdPoolCI.queueFamilyIndex = VKDevice::Get().GetPhysicalDevice()->GetGraphicsQueueFamilyIndex();
                cmdPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

                VK_CHECK_RESULT(vkCreateCommandPool(VKDevice::Get().GetDevice(), &cmdPoolCI, nullptr, &m_Frames[i].CommandPool));

                m_Frames[i].MainCommandBuffer = new VKCommandBuffer();
                m_Frames[i].MainCommandBuffer->Init(true, m_Frames[i].CommandPool);
            }

            delete[] pSwapChainImages;

            return true;
        }

        VkResult VKSwapchain::AcquireNextImage()
        {
            LUMOS_PROFILE_FUNCTION();

            {
                LUMOS_PROFILE_SCOPE("vkAcquireNextImageKHR");

                auto result = vkAcquireNextImageKHR(VKDevice::Get().GetDevice(), m_SwapChain, UINT64_MAX, GetCurrentFrameData().PresentSemaphore, VK_NULL_HANDLE, &m_AcquireImageIndex);
                
                return result;
            }
        }

        CommandBuffer* VKSwapchain::GetCurrentCommandBuffer()
        {
            return GetCurrentFrameData().MainCommandBuffer;
        }

        void VKSwapchain::Present()
        {
            LUMOS_PROFILE_FUNCTION();

            auto& frameData = GetCurrentFrameData();
            auto cmdBuffer = frameData.MainCommandBuffer->GetHandle();
            VkSubmitInfo submitInfo = {};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.pNext = VK_NULL_HANDLE;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &cmdBuffer;
            VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

            submitInfo.pWaitDstStageMask = &waitStage;

            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = &frameData.PresentSemaphore;

            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = &frameData.RenderSemaphore;
            
            {
                LUMOS_PROFILE_SCOPE("vkQueueSubmit");
                VK_CHECK_RESULT(vkQueueSubmit(VKDevice::Get().GetGraphicsQueue(), 1, &submitInfo, frameData.RenderFence->GetHandle()));
            }
            
            GetCurrentFrameData().RenderFence->Wait();
            
            if(GetCurrentFrameData().RenderFence->Signaled())
            {
                GetCurrentFrameData().MainCommandBuffer->Reset();
                GetCurrentFrameData().RenderFence->Reset();
            }
            

            VkPresentInfoKHR present;
            present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            present.pNext = VK_NULL_HANDLE;
            present.swapchainCount = 1;
            present.pSwapchains = &m_SwapChain;
            present.pImageIndices = &m_AcquireImageIndex;
            present.waitSemaphoreCount = 1;
            present.pWaitSemaphores = &frameData.RenderSemaphore;
            present.pResults = VK_NULL_HANDLE;
            auto error = vkQueuePresentKHR(VKDevice::Get().GetPresentQueue(), &present);

            if(error == VK_ERROR_OUT_OF_DATE_KHR)
            {
                LUMOS_LOG_ERROR("[Vulkan] Swapchain out of date");
            }
            else if(error == VK_SUBOPTIMAL_KHR)
            {
                LUMOS_LOG_ERROR("[Vulkan] Swapchain suboptimal");
            }
            else
            {
                VK_CHECK_RESULT(error);
            }
            
            m_CurrentBuffer = (m_CurrentBuffer + 1) % m_SwapChainBuffers.size();
        }

        void VKSwapchain::MakeDefault()
        {
            CreateFunc = CreateFuncVulkan;
        }

        Swapchain* VKSwapchain::CreateFuncVulkan(uint32_t width, uint32_t height)
        {
            return new VKSwapchain(width, height);
        }
    }

    void Graphics::VKSwapchain::FindImageFormatAndColourSpace()
    {
        LUMOS_PROFILE_FUNCTION();
        VkPhysicalDevice physicalDevice = VKDevice::Get().GetPhysicalDevice()->GetVulkanPhysicalDevice();

        // Get list of supported surface formats
        uint32_t formatCount;
        VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_Surface, &formatCount, NULL));
        LUMOS_ASSERT(formatCount > 0, "");

        std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
        VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_Surface, &formatCount, surfaceFormats.data()));

        // If the surface format list only includes one entry with VK_FORMAT_UNDEFINED,
        // there is no preferered format, so we assume VK_FORMAT_B8G8R8A8_UNORM
        if((formatCount == 1) && (surfaceFormats[0].format == VK_FORMAT_UNDEFINED))
        {
            m_ColourFormat = VK_FORMAT_B8G8R8A8_UNORM;
            m_ColourSpace = surfaceFormats[0].colorSpace;
        }
        else
        {
            // iterate over the list of available surface format and
            // check for the presence of VK_FORMAT_B8G8R8A8_UNORM
            bool found_B8G8R8A8_UNORM = false;
            for(auto&& surfaceFormat : surfaceFormats)
            {
                if(surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM)
                {
                    m_ColourFormat = surfaceFormat.format;
                    m_ColourSpace = surfaceFormat.colorSpace;
                    found_B8G8R8A8_UNORM = true;
                    break;
                }
            }

            // in case VK_FORMAT_B8G8R8A8_UNORM is not available
            // select the first available colour format
            if(!found_B8G8R8A8_UNORM)
            {
                m_ColourFormat = surfaceFormats[0].format;
                m_ColourSpace = surfaceFormats[0].colorSpace;
            }
        }
    }
}
