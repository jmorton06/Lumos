#include "LM.h"
#include "VKIMGUIRenderer.h"
#include "external/imgui/imgui.h"

#include "external/imgui/examples/imgui_impl_vulkan.h"
#include "Dependencies/vulkan/vulkan.h"

#include "VKDevice.h"
#include "VKCommandBuffer.h"
#include "VKRenderer.h"
#include "VKRenderpass.h"

static ImGui_ImplVulkanH_WindowData g_WindowData;
static VkAllocationCallbacks* g_Allocator = nullptr;
static VkDescriptorPool g_DescriptorPool = VK_NULL_HANDLE;

static void check_vk_result(VkResult err)
{
    if (err == 0) return;
    printf("VkResult %d\n", err);
    if (err < 0)
        abort();
}

namespace Lumos
{
    namespace graphics
    {
        VKIMGUIRenderer::VKIMGUIRenderer(uint width, uint height)
        {
            m_Implemented = true;
			m_WindowHandle = nullptr;
			m_Width = width;
			m_Height = height;
        }

        VKIMGUIRenderer::~VKIMGUIRenderer()
        {
            ImGui_ImplVulkan_Shutdown();
        }

        static void SetupVulkanWindowData(ImGui_ImplVulkanH_WindowData* wd, VkSurfaceKHR surface, int width, int height)
        {
            // Create Descriptor Pool
            {
                VkDescriptorPoolSize pool_sizes[] =
                {
                    { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
                    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
                    { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
                    { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
                    { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
                    { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
                    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
                    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
                    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
                    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
                    { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
                };
                VkDescriptorPoolCreateInfo pool_info = {};
                pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
                pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
                pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
                pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
                pool_info.pPoolSizes = pool_sizes;
                VkResult err = vkCreateDescriptorPool(VKDevice::Instance()->GetDevice(), &pool_info, g_Allocator, &g_DescriptorPool);
                check_vk_result(err);
            }

            wd->Surface = surface;
            wd->ClearEnable = false;

            // Check for WSI support
            VkBool32 res;
            vkGetPhysicalDeviceSurfaceSupportKHR(VKDevice::Instance()->GetGPU(), VKDevice::Instance()->GetGraphicsQueueFamilyIndex(), wd->Surface, &res);
            if (res != VK_TRUE)
            {
                fprintf(stderr, "Error no WSI support on physical device 0\n");
                exit(-1);
            }

            // Get Surface Format
            const VkFormat requestSurfaceImageFormat[] = { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM };
            const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
            wd->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(VKDevice::Instance()->GetGPU(), wd->Surface, requestSurfaceImageFormat, (size_t)IM_ARRAYSIZE(requestSurfaceImageFormat), requestSurfaceColorSpace);

            // Get Present Mode
        #ifdef IMGUI_UNLIMITED_FRAME_RATE
            VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR };
        #else
            VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_FIFO_KHR };
        #endif
            wd->PresentMode = ImGui_ImplVulkanH_SelectPresentMode(VKDevice::Instance()->GetGPU(), wd->Surface, &present_modes[0], IM_ARRAYSIZE(present_modes));

            // Create SwapChain, RenderPass, Framebuffer, etc.
            ImGui_ImplVulkanH_CreateWindowDataCommandBuffers(VKDevice::Instance()->GetGPU(), VKDevice::Instance()->GetDevice(), VKDevice::Instance()->GetGraphicsQueueFamilyIndex(), wd, g_Allocator);
            auto swapChain = ((VKSwapchain*)VKRenderer::GetRenderer()->GetSwapchain());
            wd->Swapchain = swapChain->GetSwapchain();
            wd->Width = width;
            wd->Height = height;
            VkResult err;

            wd->BackBufferCount = (size_t)swapChain->GetSwapchainBufferCount();
/*
            err = vkGetSwapchainImagesKHR(VKDevice::Instance()->GetDevice(), wd->Swapchain, &wd->BackBufferCount, NULL);
            check_vk_result(err);
            err = vkGetSwapchainImagesKHR(VKDevice::Instance()->GetDevice(), wd->Swapchain, &wd->BackBufferCount, wd->BackBuffer);
            check_vk_result(err);
            */

            // Create the Render Pass
            {
                VkAttachmentDescription attachment = {};
                attachment.format = wd->SurfaceFormat.format;
                attachment.samples = VK_SAMPLE_COUNT_1_BIT;
                attachment.loadOp = wd->ClearEnable ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                VkAttachmentReference color_attachment = {};
                color_attachment.attachment = 0;
                color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                VkSubpassDescription subpass = {};
                subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
                subpass.colorAttachmentCount = 1;
                subpass.pColorAttachments = &color_attachment;
                VkSubpassDependency dependency = {};
                dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
                dependency.dstSubpass = 0;
                dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dependency.srcAccessMask = 0;
                dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                VkRenderPassCreateInfo info = {};
                info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
                info.attachmentCount = 1;
                info.pAttachments = &attachment;
                info.subpassCount = 1;
                info.pSubpasses = &subpass;
                info.dependencyCount = 1;
                info.pDependencies = &dependency;
                err = vkCreateRenderPass(VKDevice::Instance()->GetDevice(), &info, NULL, &wd->RenderPass);
                check_vk_result(err);

                //auto Renderp = ((VKSwapchain*)VKRenderer::GetRenderer()->GetSwapchain())->GetRenderPass();
               // wd->RenderPass = ((VKRenderpass*)Renderp)->GetRenderpass();
            }

            // Create The Image Views
            {
                /*VkImageViewCreateInfo info = {};
                info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                info.viewType = VK_IMAGE_VIEW_TYPE_2D;
                info.format = wd->SurfaceFormat.format;
                info.components.r = VK_COMPONENT_SWIZZLE_R;
                info.components.g = VK_COMPONENT_SWIZZLE_G;
                info.components.b = VK_COMPONENT_SWIZZLE_B;
                info.components.a = VK_COMPONENT_SWIZZLE_A;
                VkImageSubresourceRange image_range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
                info.subresourceRange = image_range;*/
                for (uint32_t i = 0; i < wd->BackBufferCount; i++)
                {
                    auto scBuffer = swapChain->GetTexture(i);
                    wd->BackBuffer[i] = scBuffer->GetImage();
                    wd->BackBufferView[i] = scBuffer->GetImageView();
                    //info.image = wd->BackBuffer[i];
                    //err = vkCreateImageView(VKDevice::Instance()->GetDevice(), &info, NULL, &wd->BackBufferView[i]);
                    //check_vk_result(err);
                }
            }

            // Create Framebuffer
            {
                VkImageView attachment[1];
                VkFramebufferCreateInfo info = {};
                info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                info.renderPass = wd->RenderPass;
                info.attachmentCount = 1;
                info.pAttachments = attachment;
                info.width = wd->Width;
                info.height = wd->Height;
                info.layers = 1;
                for (uint32_t i = 0; i < wd->BackBufferCount; i++)
                {
                    attachment[0] = wd->BackBufferView[i];
                    err = vkCreateFramebuffer(VKDevice::Instance()->GetDevice(), &info, NULL, &wd->Framebuffer[i]);
                    check_vk_result(err);
                }
            }
        }

        void VKIMGUIRenderer::Init()
        {
            //VKSwapchain* swapChain = (VKSwapchain*)VKRenderer::GetRenderer()->GetSwapchain();

            int w, h;
            w = (int)m_Width;
            h = (int)m_Height;
            ImGui_ImplVulkanH_WindowData* wd = &g_WindowData;
            VkSurfaceKHR surface = VKDevice::Instance()->GetSurface();
            SetupVulkanWindowData(wd, surface, w, h);

            // Setup Vulkan binding
            ImGui_ImplVulkan_InitInfo init_info = {};
            init_info.Instance = ((VKContext*)VKContext::GetContext())->GetVKInstance();
            init_info.PhysicalDevice = VKDevice::Instance()->GetGPU();
            init_info.Device = VKDevice::Instance()->GetDevice();
            init_info.QueueFamily = VKDevice::Instance()->GetGraphicsQueueFamilyIndex();
            init_info.Queue = VKDevice::Instance()->GetGraphicsQueue();
            init_info.PipelineCache = VKDevice::Instance()->GetPipelineCache();
            init_info.DescriptorPool = g_DescriptorPool;
            init_info.Allocator = g_Allocator;
            init_info.CheckVkResultFn = NULL;
            ImGui_ImplVulkan_Init(&init_info, wd->RenderPass);

            VkResult err;
            // Upload Fonts
            {
                // Use any command queue
                VkCommandPool command_pool = wd->Frames[wd->FrameIndex].CommandPool;
                VkCommandBuffer command_buffer = wd->Frames[wd->FrameIndex].CommandBuffer;

                err = vkResetCommandPool(VKDevice::Instance()->GetDevice(), command_pool, 0);
                check_vk_result(err);
                VkCommandBufferBeginInfo begin_info = {};
                begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
                err = vkBeginCommandBuffer(command_buffer, &begin_info);
                check_vk_result(err);

                ImGui_ImplVulkan_CreateFontsTexture(command_buffer);

                VkSubmitInfo end_info = {};
                end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                end_info.commandBufferCount = 1;
                end_info.pCommandBuffers = &command_buffer;
                err = vkEndCommandBuffer(command_buffer);
                check_vk_result(err);
                err = vkQueueSubmit(VKDevice::Instance()->GetGraphicsQueue(), 1, &end_info, VK_NULL_HANDLE);
                check_vk_result(err);

                err = vkDeviceWaitIdle(VKDevice::Instance()->GetDevice());
                check_vk_result(err);
                ImGui_ImplVulkan_InvalidateFontUploadObjects();
            }
        }

        void VKIMGUIRenderer::NewFrame()
        {
        }

        static void FrameRender(ImGui_ImplVulkanH_WindowData* wd)
        {
            VkResult err;

            VkSemaphore& image_acquired_semaphore  = wd->Frames[wd->FrameIndex].ImageAcquiredSemaphore;
            err = vkAcquireNextImageKHR(VKDevice::Instance()->GetDevice(), wd->Swapchain, UINT64_MAX, image_acquired_semaphore, VK_NULL_HANDLE, &wd->FrameIndex);
            check_vk_result(err);

            ImGui_ImplVulkanH_FrameData* fd = &wd->Frames[wd->FrameIndex];
            {
                err = vkWaitForFences(VKDevice::Instance()->GetDevice(), 1, &fd->Fence, VK_TRUE, UINT64_MAX);	// wait indefinitely instead of periodically checking
                check_vk_result(err);

                err = vkResetFences(VKDevice::Instance()->GetDevice(), 1, &fd->Fence);
                check_vk_result(err);
            }
            {
                err = vkResetCommandPool(VKDevice::Instance()->GetDevice(), fd->CommandPool, 0);
                check_vk_result(err);
                VkCommandBufferBeginInfo info = {};
                info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
                err = vkBeginCommandBuffer(fd->CommandBuffer, &info);
                check_vk_result(err);
            }
            {
                VkRenderPassBeginInfo info = {};
                info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                info.renderPass = wd->RenderPass;
                info.framebuffer = wd->Framebuffer[wd->FrameIndex];
                info.renderArea.extent.width = wd->Width;
                info.renderArea.extent.height = wd->Height;
                info.clearValueCount = 1;
                info.pClearValues = &wd->ClearValue;
                vkCmdBeginRenderPass(fd->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
            }

            // Record Imgui Draw Data and draw funcs into command buffer
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), fd->CommandBuffer);

            // Submit command buffer
            vkCmdEndRenderPass(fd->CommandBuffer);
            {
                VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                VkSubmitInfo info = {};
                info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                info.waitSemaphoreCount = 1;
                info.pWaitSemaphores = &image_acquired_semaphore;
                info.pWaitDstStageMask = &wait_stage;
                info.commandBufferCount = 1;
                info.pCommandBuffers = &fd->CommandBuffer;
                info.signalSemaphoreCount = 1;
                info.pSignalSemaphores = &fd->RenderCompleteSemaphore;

                err = vkEndCommandBuffer(fd->CommandBuffer);
                check_vk_result(err);
                err = vkQueueSubmit(VKDevice::Instance()->GetGraphicsQueue(), 1, &info, fd->Fence);
                check_vk_result(err);
            }
        }

        static void FramePresent(ImGui_ImplVulkanH_WindowData* wd)
        {
            ImGui_ImplVulkanH_FrameData* fd = &wd->Frames[wd->FrameIndex];
            VkPresentInfoKHR info = {};
            info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            info.waitSemaphoreCount = 1;
            info.pWaitSemaphores = &fd->RenderCompleteSemaphore;
            info.swapchainCount = 1;
            info.pSwapchains = &wd->Swapchain;
            info.pImageIndices = &wd->FrameIndex;
            VkResult err = vkQueuePresentKHR(VKDevice::Instance()->GetGraphicsQueue(), &info);
            check_vk_result(err);
        }

        void VKIMGUIRenderer::Render(Lumos::graphics::api::CommandBuffer* commandBuffer)
        {
            //g_WindowData.FrameIndex = ((VKSwapchain*)VKRenderer::GetRenderer()->GetSwapchain())->GetCurrentBufferId();
           // ImGui_ImplVulkanH_FrameData* fd = &g_WindowData.Frames[g_WindowData.FrameIndex];
            //FrameRender(&g_WindowData);
          //  ((VKSwapchain*)VKRenderer::GetRenderer()->GetSwapchain())->Present(fd->RenderCompleteSemaphore);
            //FramePresent(&g_WindowData);
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), ((VKCommandBuffer*)commandBuffer)->GetCommandBuffer());
        }
    }
}

