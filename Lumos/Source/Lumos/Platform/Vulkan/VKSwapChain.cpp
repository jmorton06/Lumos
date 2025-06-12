#include "Precompiled.h"
#include "VKDevice.h"
#include "VKSwapChain.h"
#include "VKUtilities.h"
#include "VKRenderer.h"
#include "VKSemaphore.h"
#include "VKTexture.h"
#include "Core/Application.h"
#include "Core/Thread.h"
#include "Maths/MathsUtilities.h"

namespace Lumos
{
    namespace Graphics
    {
        VKSwapChain::VKSwapChain(uint32_t width, uint32_t height)
            : m_SwapChainBufferCount(0)
        {
            m_Width        = width;
            m_Height       = height;
            m_SwapChain    = VK_NULL_HANDLE;
            m_OldSwapChain = VK_NULL_HANDLE;
            m_Surface      = VK_NULL_HANDLE;

            // Initialised by first Image Acquire
            m_CurrentBuffer     = 0; // std::numeric_limits<uint32_t>::max();
            m_AcquireImageIndex = std::numeric_limits<uint32_t>::max();
        }

        VKSwapChain::~VKSwapChain()
        {
            vkDeviceWaitIdle(VKDevice::Get().GetDevice());

            for(uint32_t i = 0; i < m_SwapChainBufferCount; i++)
            {
                if(m_Frames[i].MainCommandBuffer)
                    m_Frames[i].MainCommandBuffer->Flush();

                m_Frames[i].MainCommandBuffer     = nullptr;
                m_Frames[i].CommandPool           = nullptr;
                m_Frames[i].ImageAcquireSemaphore = nullptr;

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

            if(!success)
                LERROR("Failed to initialise swapchain");

            // AcquireNextImage();

            return success;
        }

        bool VKSwapChain::Init(bool vsync)
        {
            LUMOS_PROFILE_FUNCTION();
            FindImageFormatAndColourSpace();

            if(!m_Surface)
            {
                LFATAL("[VULKAN] Failed to create window surface!");
            }

            VkBool32 queueIndexSupported;
            vkGetPhysicalDeviceSurfaceSupportKHR(VKDevice::Get().GetPhysicalDevice()->GetHandle(), VKDevice::Get().GetPhysicalDevice()->GetGraphicsQueueFamilyIndex(), m_Surface, &queueIndexSupported);

            if(queueIndexSupported == VK_FALSE)
                LERROR("Present Queue not supported");

            // Swap chain
            VkSurfaceCapabilitiesKHR surfaceCapabilities;
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VKDevice::Get().GetGPU(), m_Surface, &surfaceCapabilities);

            uint32_t numPresentModes;
            vkGetPhysicalDeviceSurfacePresentModesKHR(VKDevice::Get().GetGPU(), m_Surface, &numPresentModes, VK_NULL_HANDLE);

            TDArray<VkPresentModeKHR> pPresentModes(numPresentModes);
            vkGetPhysicalDeviceSurfacePresentModesKHR(VKDevice::Get().GetGPU(), m_Surface, &numPresentModes, pPresentModes.Data());

            VkExtent2D swapChainExtent;

            swapChainExtent.width  = static_cast<uint32_t>(m_Width);
            swapChainExtent.height = static_cast<uint32_t>(m_Height);

            VkPresentModeKHR swapChainPresentMode = VKUtilities::ChoosePresentMode(pPresentModes, vsync);

            // Use triple-buffering
            m_SwapChainBufferCount = surfaceCapabilities.maxImageCount;

            if(m_SwapChainBufferCount > 3)
                m_SwapChainBufferCount = 3;
            else if(m_SwapChainBufferCount == 0)
                m_SwapChainBufferCount = 3;

            ASSERT(m_SwapChainBufferCount > 1);

            VkSurfaceTransformFlagBitsKHR preTransform;
            if(surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
                preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
            else
                preTransform = surfaceCapabilities.currentTransform;

            VkCompositeAlphaFlagBitsKHR compositeAlpha               = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
            TDArray<VkCompositeAlphaFlagBitsKHR> compositeAlphaFlags = {
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
                    m_Frames[i].ImageAcquireSemaphore = VK_NULL_HANDLE;
                }

                MemorySet(m_SwapChainBuffers, 0, sizeof(Texture*) * MAX_SWAPCHAIN_BUFFERS);

                vkDestroySwapchainKHR(VKDevice::Get().GetDevice(), m_OldSwapChain, VK_NULL_HANDLE);
                m_OldSwapChain = VK_NULL_HANDLE;
            }

            uint32_t swapChainImageCount;
            VK_CHECK_RESULT(vkGetSwapchainImagesKHR(VKDevice::Get().GetDevice(), m_SwapChain, &swapChainImageCount, VK_NULL_HANDLE));

            ArenaTemp scratch         = ScratchBegin(nullptr, 0);
            VkImage* pSwapChainImages = PushArrayNoZero(scratch.arena, VkImage, swapChainImageCount);
            VK_CHECK_RESULT(vkGetSwapchainImagesKHR(VKDevice::Get().GetDevice(), m_SwapChain, &swapChainImageCount, pSwapChainImages));

            m_SwapChainBufferCount = swapChainImageCount;

            for(uint32_t i = 0; i < m_SwapChainBufferCount; i++)
            {
                VkImageViewCreateInfo viewCI {};
                viewCI.sType  = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                viewCI.format = m_ColourFormat;
#if defined(LUMOS_PLATFORM_MACOS) || defined(LUMOS_PLATFORM_IOS)
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

                m_SwapChainBuffers[i] = swapChainBuffer;
            }

            ScratchEnd(scratch);

            CreateFrameData();

            return true;
        }

        FrameData& VKSwapChain::GetCurrentFrameData()
        {
            ASSERT(m_CurrentBuffer < m_SwapChainBufferCount, "Incorrect swapchain buffer index");
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

                m_Frames[i].ImageAcquireSemaphore = CreateSharedPtr<VKSemaphore>(false);
                if(!m_Frames[i].MainCommandBuffer)
                {
                    m_Frames[i].CommandPool = CreateSharedPtr<VKCommandPool>(VKDevice::Get().GetPhysicalDevice()->GetGraphicsQueueFamilyIndex(), VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

                    m_Frames[i].MainCommandBuffer = CreateSharedPtr<VKCommandBuffer>();
                    m_Frames[i].MainCommandBuffer->Init(true, m_Frames[i].CommandPool->GetHandle());
                    ArenaTemp scratch         = ScratchBegin(nullptr, 0);
                    String8 commandBufferName = PushStr8F(scratch.arena, "Commandbuffer - Frame %u", i);
                    VKUtilities::SetDebugUtilsObjectName(VKDevice::Get().GetDevice(), VK_OBJECT_TYPE_COMMAND_BUFFER, ToCChar(commandBufferName), m_Frames[i].MainCommandBuffer->GetHandle());
                    ScratchEnd(scratch);
                }
            }
        }

        bool VKSwapChain::AcquireNextImage()
        {
            LUMOS_PROFILE_FUNCTION();

            static int FailedCount = 0;

            if(m_SwapChainBufferCount == 1 && m_AcquireImageIndex != std::numeric_limits<uint32_t>::max())
                return true;

            while(FailedCount < 10)
            {
                LUMOS_PROFILE_SCOPE("vkAcquireNextImageKHR");
                auto result = vkAcquireNextImageKHR(VKDevice::Get().GetDevice(), m_SwapChain, UINT64_MAX, m_Frames[m_CurrentBuffer].ImageAcquireSemaphore->GetHandle(), VK_NULL_HANDLE, &m_AcquireImageIndex);

                if(result == VK_SUCCESS)
                {
                    return true;
                }

                if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
                {
                    LINFO("Acquire Image result : %s", result == VK_ERROR_OUT_OF_DATE_KHR ? "Out of Date" : "SubOptimal");

                    if(result == VK_ERROR_OUT_OF_DATE_KHR)
                    {
                        OnResize(m_Width, m_Height, true);
#ifdef LUMOS_PLATFORM_LINUX
                        return false;
#endif
                    }

                    return true;
                }

                if(result == VK_NOT_READY)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    FailedCount++;
                    LFATAL("[VULKAN] Failed to acquire swap chain image! - %s", VKUtilities::ErrorString(result).c_str());
                }
            }

            LFATAL("[VULKAN] Failed to acquire swap chain image %i times! - Exiting", FailedCount);
            Application::Get().SetAppState(AppState::Closing);
            return false;
        }

        void VKSwapChain::OnResize(uint32_t width, uint32_t height, bool forceResize, Window* windowHandle)
        {
            LUMOS_PROFILE_FUNCTION();

            if(!forceResize && m_Width == width && m_Height == height)
                return;

            VKRenderer::GetGraphicsContext()->WaitIdle();

            m_Width  = Maths::Max(width, 1.0f);
            m_Height = Maths::Max(height, 1.0f);

            m_OldSwapChain = m_SwapChain;
            m_SwapChain    = VK_NULL_HANDLE;

            if(windowHandle)
                Init(m_VSyncEnabled, windowHandle);
            else
            {
                Init(m_VSyncEnabled);
            }
        }

        void VKSwapChain::QueueSubmit()
        {
            LUMOS_PROFILE_FUNCTION();
            auto& frameData = GetCurrentFrameData();
            frameData.MainCommandBuffer->Execute(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, frameData.ImageAcquireSemaphore->GetHandle(), true);
        }

        CommandBuffer* VKSwapChain::GetCurrentCommandBuffer()
        {
            return GetCurrentFrameData().MainCommandBuffer.get();
        }

        bool VKSwapChain::Begin()
        {
            LUMOS_PROFILE_FUNCTION();
            m_CurrentBuffer = (m_CurrentBuffer + 1) % m_SwapChainBufferCount;

            auto commandBuffer = GetCurrentFrameData().MainCommandBuffer;
            if(commandBuffer->GetState() == CommandBufferState::Submitted)
            {
                if(!commandBuffer->Wait())
                {
                    LFATAL("Command buffer wait failed — GPU sync issue");
                    Application::Get().SetAppState(AppState::Closing);
                    return false;
                }
            }

            commandBuffer->Reset();
            VKRenderer::GetDeletionQueue(m_CurrentBuffer).Flush();

            if(!AcquireNextImage())
            {
                LWARN("Failed to AcquireNextImage");
                return false;
            }

            commandBuffer->BeginRecording();

            return true;
        }

        void VKSwapChain::End()
        {
            LUMOS_PROFILE_FUNCTION();

            GetCurrentCommandBuffer()->EndRecording();
        }

        void VKSwapChain::Present(const TDArray<VkSemaphore>& semaphores)
        {
            LUMOS_PROFILE_FUNCTION();

            VkSemaphore vkWaitSemaphores[3] = { nullptr, nullptr, nullptr };
            uint32_t semaphoreCount         = 0;

            for(auto semaphore : semaphores)
            {
                if(semaphoreCount >= 3)
                    break;

                vkWaitSemaphores[semaphoreCount++] = semaphore;
            }

            VkPresentInfoKHR present;
            present.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            present.pNext              = VK_NULL_HANDLE;
            present.swapchainCount     = 1;
            present.pSwapchains        = &m_SwapChain;
            present.pImageIndices      = &m_AcquireImageIndex;
            present.waitSemaphoreCount = semaphoreCount;
            present.pWaitSemaphores    = vkWaitSemaphores;
            present.pResults           = VK_NULL_HANDLE;

            auto error = vkQueuePresentKHR(VKDevice::Get().GetPresentQueue(), &present);

            if(error == VK_ERROR_OUT_OF_DATE_KHR)
            {
                LWARN("[Vulkan] SwapChain out of date");
            }
            else if(error == VK_SUBOPTIMAL_KHR)
            {
                LWARN("[Vulkan] SwapChain suboptimal");
            }
            else
            {
                VK_CHECK_RESULT(error);
            }

            // AcquireNextImage();
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
        ASSERT(formatCount > 0);

        TDArray<VkSurfaceFormatKHR> surfaceFormats(formatCount);
        VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_Surface, &formatCount, surfaceFormats.Data()));

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
