#include "Precompiled.h"
#include "VKDevice.h"
#include "VKSwapChain.h"
#include "VKUtilities.h"
#include "VKFence.h"
#include "VKRenderer.h"
#include "Core/Application.h"

namespace Lumos
{
    namespace Graphics
    {
        VKSwapChain::VKSwapChain(uint32_t width, uint32_t height)
        {
            m_Width        = width;
            m_Height       = height;
            m_SwapChain    = VK_NULL_HANDLE;
            m_OldSwapChain = VK_NULL_HANDLE;
            m_Surface      = VK_NULL_HANDLE;

            // Initialised by first Image Aquire
            m_CurrentBuffer     = 0; // std::numeric_limits<uint32_t>::max();
            m_AcquireImageIndex = std::numeric_limits<uint32_t>::max();
        }

        VKSwapChain::~VKSwapChain()
        {
            vkDeviceWaitIdle(VKDevice::Get().GetDevice());

            for(uint32_t i = 0; i < m_SwapChainBufferCount; i++)
            {
                vkDestroySemaphore(VKDevice::Get().GetDevice(), m_Frames[i].PresentSemaphore, nullptr);
                m_Frames[i].MainCommandBuffer->Flush();

                m_Frames[i].MainCommandBuffer = nullptr;
                m_Frames[i].CommandPool       = nullptr;

                delete m_SwapChainBuffers[i];
            }
            vkDestroySwapchainKHR(VKDevice::Get().GetDevice(), m_SwapChain, VK_NULL_HANDLE);

            if(m_Surface != VK_NULL_HANDLE)
            {
                vkDestroySurfaceKHR(VKContext::GetVKInstance(), m_Surface, nullptr);
            }
        }

        bool VKSwapChain::Init(bool vsync, Window* windowHandle)
        {
            LUMOS_PROFILE_FUNCTION();
            m_VSyncEnabled = vsync;

            if(m_Surface == VK_NULL_HANDLE)
                m_Surface = CreatePlatformSurface(VKContext::GetVKInstance(), windowHandle);

            bool success = Init(m_VSyncEnabled);

            // AcquireNextImage();

            return success;
        }

        bool VKSwapChain::Init(bool vsync)
        {
            LUMOS_PROFILE_FUNCTION();
            FindImageFormatAndColourSpace();

            if(!m_Surface)
            {
                LUMOS_LOG_CRITICAL("[VULKAN] Failed to create window surface!");
            }

            VkBool32 queueIndexSupported;
            vkGetPhysicalDeviceSurfaceSupportKHR(VKDevice::Get().GetPhysicalDevice()->GetHandle(), VKDevice::Get().GetPhysicalDevice()->GetGraphicsQueueFamilyIndex(), m_Surface, &queueIndexSupported);

            if(queueIndexSupported == VK_FALSE)
                LUMOS_LOG_ERROR("Present Queue not supported");

            // Swap chain
            VkSurfaceCapabilitiesKHR surfaceCapabilities;
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VKDevice::Get().GetGPU(), m_Surface, &surfaceCapabilities);

            uint32_t numPresentModes;
            vkGetPhysicalDeviceSurfacePresentModesKHR(VKDevice::Get().GetGPU(), m_Surface, &numPresentModes, VK_NULL_HANDLE);

            std::vector<VkPresentModeKHR> pPresentModes(numPresentModes);
            vkGetPhysicalDeviceSurfacePresentModesKHR(VKDevice::Get().GetGPU(), m_Surface, &numPresentModes, pPresentModes.data());

            VkExtent2D swapChainExtent;

            swapChainExtent.width  = static_cast<uint32_t>(m_Width);
            swapChainExtent.height = static_cast<uint32_t>(m_Height);

            VkPresentModeKHR swapChainPresentMode = VKUtilities::ChoosePresentMode(pPresentModes, vsync);

            // Use triple-buffering
            m_SwapChainBufferCount = surfaceCapabilities.maxImageCount;

            if(m_SwapChainBufferCount > 3)
                m_SwapChainBufferCount = 3;

            VkSurfaceTransformFlagBitsKHR preTransform;
            if(surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
                preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
            else
                preTransform = surfaceCapabilities.currentTransform;

            VkCompositeAlphaFlagBitsKHR compositeAlpha                   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
            std::vector<VkCompositeAlphaFlagBitsKHR> compositeAlphaFlags = {
                VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
                VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
                VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
            };
            for(auto& compositeAlphaFlag : compositeAlphaFlags)
            {
                if(surfaceCapabilities.supportedCompositeAlpha & compositeAlphaFlag)
                {
                    compositeAlpha = compositeAlphaFlag;
                    break;
                };
            }

            VkSwapchainCreateInfoKHR swapChainCI {};
            swapChainCI.sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            swapChainCI.surface               = m_Surface;
            swapChainCI.minImageCount         = m_SwapChainBufferCount;
            swapChainCI.imageFormat           = m_ColourFormat;
            swapChainCI.imageExtent.width     = swapChainExtent.width;
            swapChainCI.imageExtent.height    = swapChainExtent.height;
            swapChainCI.preTransform          = preTransform;
            swapChainCI.compositeAlpha        = compositeAlpha;
            swapChainCI.imageArrayLayers      = 1;
            swapChainCI.presentMode           = swapChainPresentMode;
            swapChainCI.oldSwapchain          = m_OldSwapChain;
            swapChainCI.imageColorSpace       = m_ColourSpace;
            swapChainCI.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
            swapChainCI.queueFamilyIndexCount = 0;
            swapChainCI.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            swapChainCI.pQueueFamilyIndices   = VK_NULL_HANDLE;
            swapChainCI.clipped               = VK_TRUE;

            if(surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
            {
                swapChainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
            }

            if(surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
            {
                swapChainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            }

            VK_CHECK_RESULT(vkCreateSwapchainKHR(VKDevice::Get().GetDevice(), &swapChainCI, VK_NULL_HANDLE, &m_SwapChain));

            if(m_OldSwapChain != VK_NULL_HANDLE)
            {
                for(uint32_t i = 0; i < m_SwapChainBufferCount; i++)
                {
                    if(m_Frames[i].MainCommandBuffer->GetState() == CommandBufferState::Submitted)
                        m_Frames[i].MainCommandBuffer->Wait();

                    m_Frames[i].MainCommandBuffer->Reset();

                    delete m_SwapChainBuffers[i];
                    vkDestroySemaphore(VKDevice::Get().GetDevice(), m_Frames[i].PresentSemaphore, nullptr);
                    m_Frames[i].PresentSemaphore = VK_NULL_HANDLE;
                }

                m_SwapChainBuffers.clear();

                vkDestroySwapchainKHR(VKDevice::Get().GetDevice(), m_OldSwapChain, VK_NULL_HANDLE);
                m_OldSwapChain = VK_NULL_HANDLE;
            }

            uint32_t swapChainImageCount;
            VK_CHECK_RESULT(vkGetSwapchainImagesKHR(VKDevice::Get().GetDevice(), m_SwapChain, &swapChainImageCount, VK_NULL_HANDLE));

            VkImage* pSwapChainImages = new VkImage[swapChainImageCount];
            VK_CHECK_RESULT(vkGetSwapchainImagesKHR(VKDevice::Get().GetDevice(), m_SwapChain, &swapChainImageCount, pSwapChainImages));

            for(uint32_t i = 0; i < m_SwapChainBufferCount; i++)
            {
                VkImageViewCreateInfo viewCI {};
                viewCI.sType  = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                viewCI.format = m_ColourFormat;
#ifdef LUMOS_PLATFORM_MACOS
                viewCI.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
#else
                viewCI.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
#endif
                viewCI.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
                viewCI.subresourceRange.baseMipLevel   = 0;
                viewCI.subresourceRange.levelCount     = 1;
                viewCI.subresourceRange.baseArrayLayer = 0;
                viewCI.subresourceRange.layerCount     = 1;
                viewCI.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
                viewCI.flags                           = 0;
                viewCI.image                           = pSwapChainImages[i];

                VkImageView imageView;
                VK_CHECK_RESULT(vkCreateImageView(VKDevice::Get().GetDevice(), &viewCI, VK_NULL_HANDLE, &imageView));
                VKTexture2D* swapChainBuffer = new VKTexture2D(pSwapChainImages[i], imageView, m_ColourFormat, m_Width, m_Height);
                swapChainBuffer->TransitionImage(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

                m_SwapChainBuffers.push_back(swapChainBuffer);
            }

            delete[] pSwapChainImages;

            CreateFrameData();

            return true;
        }

        FrameData& VKSwapChain::GetCurrentFrameData()
        {
            LUMOS_ASSERT(m_CurrentBuffer < m_SwapChainBufferCount, "Incorrect swapchain buffer index");
            return m_Frames[m_CurrentBuffer];
        }

        void VKSwapChain::CreateFrameData()
        {
            for(uint32_t i = 0; i < m_SwapChainBufferCount; i++)
            {
                VkSemaphoreCreateInfo semaphoreInfo = {};
                semaphoreInfo.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
                semaphoreInfo.pNext                 = nullptr;
                semaphoreInfo.flags                 = 0;

                if(m_Frames[i].PresentSemaphore == VK_NULL_HANDLE)
                    VK_CHECK_RESULT(vkCreateSemaphore(VKDevice::Get().GetDevice(), &semaphoreInfo, nullptr, &m_Frames[i].PresentSemaphore));

                if(!m_Frames[i].MainCommandBuffer)
                {
                    m_Frames[i].CommandPool = CreateSharedPtr<VKCommandPool>(VKDevice::Get().GetPhysicalDevice()->GetGraphicsQueueFamilyIndex(), VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

                    m_Frames[i].MainCommandBuffer = CreateSharedPtr<VKCommandBuffer>();
                    m_Frames[i].MainCommandBuffer->Init(true, m_Frames[i].CommandPool->GetHandle());
                    VKUtilities::SetDebugUtilsObjectName(VKDevice::Get().GetDevice(), VK_OBJECT_TYPE_COMMAND_BUFFER, fmt::format("Commandbuffer (frame in flight: {})", i), m_Frames[i].MainCommandBuffer->GetHandle());
                }
            }
        }

        void VKSwapChain::AcquireNextImage()
        {
            LUMOS_PROFILE_FUNCTION();

            static int FailedCount = 0;

            if(m_SwapChainBufferCount == 1 && m_AcquireImageIndex != std::numeric_limits<uint32_t>::max())
                return;

            {
                LUMOS_PROFILE_SCOPE("vkAcquireNextImageKHR");
                auto result = vkAcquireNextImageKHR(VKDevice::Get().GetDevice(), m_SwapChain, UINT64_MAX, m_Frames[m_CurrentBuffer].PresentSemaphore, VK_NULL_HANDLE, &m_AcquireImageIndex);

                if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
                {
                    LUMOS_LOG_INFO("Acquire Image result : {0}", result == VK_ERROR_OUT_OF_DATE_KHR ? "Out of Date" : "SubOptimal");

                    if(result == VK_ERROR_OUT_OF_DATE_KHR)
                    {
                        OnResize(m_Width, m_Height, true);
                    }
                }
                else if(result != VK_SUCCESS)
                {
                    FailedCount++;
                    LUMOS_LOG_CRITICAL("[VULKAN] Failed to acquire swap chain image! - {0}", VKUtilities::ErrorString(result));

                    if(FailedCount > 10)
                    {
                        LUMOS_LOG_CRITICAL("[VULKAN] Failed to acquire swap chain image {0} times! - Exiting", FailedCount);
                        Application::Get().SetAppState(AppState::Closing);
                    }

                    return;
                }

                FailedCount = 0;
            }
        }

        void VKSwapChain::OnResize(uint32_t width, uint32_t height, bool forceResize, Window* windowHandle)
        {
            LUMOS_PROFILE_FUNCTION();

            if(!forceResize && m_Width == width && m_Height == height)
                return;

            VKRenderer::GetGraphicsContext()->WaitIdle();

            m_Width  = width;
            m_Height = height;

            m_OldSwapChain = m_SwapChain;
            m_SwapChain    = VK_NULL_HANDLE;

            if(windowHandle)
                Init(m_VSyncEnabled, windowHandle);
            else
            {
                Init(m_VSyncEnabled);
            }

            VKRenderer::GetGraphicsContext()->WaitIdle();
        }

        void VKSwapChain::QueueSubmit()
        {
            LUMOS_PROFILE_FUNCTION();
            auto& frameData = GetCurrentFrameData();
            frameData.MainCommandBuffer->Execute(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, frameData.PresentSemaphore, true);
        }

        CommandBuffer* VKSwapChain::GetCurrentCommandBuffer()
        {
            return GetCurrentFrameData().MainCommandBuffer.get();
        }

        void VKSwapChain::Begin()
        {
            LUMOS_PROFILE_FUNCTION();
            m_CurrentBuffer = (m_CurrentBuffer + 1) % m_SwapChainBufferCount;

            auto commandBuffer = GetCurrentFrameData().MainCommandBuffer;
            if(commandBuffer->GetState() == CommandBufferState::Submitted)
            {
                if(!commandBuffer->Wait())
                {
                    return;
                }
            }
            commandBuffer->Reset();
            VKRenderer::GetDeletionQueue(m_CurrentBuffer).Flush();
            AcquireNextImage();

            commandBuffer->BeginRecording();
        }

        void VKSwapChain::End()
        {
            LUMOS_PROFILE_FUNCTION();

            GetCurrentCommandBuffer()->EndRecording();
        }

        void VKSwapChain::Present(VkSemaphore semaphore)
        {
            LUMOS_PROFILE_FUNCTION();

            VkPresentInfoKHR present;
            present.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            present.pNext              = VK_NULL_HANDLE;
            present.swapchainCount     = 1;
            present.pSwapchains        = &m_SwapChain;
            present.pImageIndices      = &m_AcquireImageIndex;
            present.waitSemaphoreCount = 1;
            present.pWaitSemaphores    = &semaphore;
            present.pResults           = VK_NULL_HANDLE;

            auto error = vkQueuePresentKHR(VKDevice::Get().GetPresentQueue(), &present);

            if(error == VK_ERROR_OUT_OF_DATE_KHR)
            {
                LUMOS_LOG_ERROR("[Vulkan] SwapChain out of date");
            }
            else if(error == VK_SUBOPTIMAL_KHR)
            {
                LUMOS_LOG_ERROR("[Vulkan] SwapChain suboptimal");
            }
            else
            {
                VK_CHECK_RESULT(error);
            }
        }

        void VKSwapChain::MakeDefault()
        {
            CreateFunc = CreateFuncVulkan;
        }

        SwapChain* VKSwapChain::CreateFuncVulkan(uint32_t width, uint32_t height)
        {
            return new VKSwapChain(width, height);
        }
    }

    void Graphics::VKSwapChain::FindImageFormatAndColourSpace()
    {
        LUMOS_PROFILE_FUNCTION();
        VkPhysicalDevice physicalDevice = VKDevice::Get().GetPhysicalDevice()->GetHandle();

        // Get list of supported surface formats
        uint32_t formatCount;
        VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_Surface, &formatCount, NULL));
        LUMOS_ASSERT(formatCount > 0);

        std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
        VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_Surface, &formatCount, surfaceFormats.data()));

        // If the surface format list only includes one entry with VK_FORMAT_UNDEFINED,
        // there is no preferered format, so we assume VK_FORMAT_B8G8R8A8_UNORM
        if((formatCount == 1) && (surfaceFormats[0].format == VK_FORMAT_UNDEFINED))
        {
            m_ColourFormat = VK_FORMAT_B8G8R8A8_UNORM;
            m_ColourSpace  = surfaceFormats[0].colorSpace;
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
                    m_ColourFormat       = surfaceFormat.format;
                    m_ColourSpace        = surfaceFormat.colorSpace;
                    found_B8G8R8A8_UNORM = true;
                    break;
                }
            }

            // in case VK_FORMAT_B8G8R8A8_UNORM is not available
            // select the first available colour format
            if(!found_B8G8R8A8_UNORM)
            {
                m_ColourFormat = surfaceFormats[0].format;
                m_ColourSpace  = surfaceFormats[0].colorSpace;
            }
        }
    }
}
