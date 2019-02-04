#include "LM.h"
#include "VKIMGUIRenderer.h"
#include "external/imgui/imgui.h"
#include "external/imgui/examples/imgui_impl_vulkan.h"

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
			for (int i = 0; i < IMGUI_VK_QUEUED_FRAMES; i++)
			{
				delete m_CommandBuffers[i];
				delete m_Framebuffers[i];
			}
               
			delete m_Renderpass;

            ImGui_ImplVulkan_Shutdown();
        }

        void VKIMGUIRenderer::SetupVulkanWindowData(ImGui_ImplVulkanH_WindowData* wd, VkSurfaceKHR surface, int width, int height)
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
            
            VkResult err;
            for (int i = 0; i < IMGUI_VK_QUEUED_FRAMES; i++)
            {
                VKCommandBuffer* commandBuffer = new VKCommandBuffer();
                commandBuffer->Init(true);
                m_CommandBuffers[i] = commandBuffer;
                
                ImGui_ImplVulkanH_FrameData* fd = &wd->Frames[i];
                {
                    //fd->CommandPool = VKDevice::Instance()->GetVKContext()->GetCommandPool()->GetCommandPool();
                    //fd->CommandBuffer = commandBuffer->GetCommandBuffer();
                    //fd->Fence = commandBuffer->GetFence();
                }
                {
                    VkSemaphoreCreateInfo info = {};
                    info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
                    err = vkCreateSemaphore(VKDevice::Instance()->GetDevice(), &info, nullptr, &fd->ImageAcquiredSemaphore);
                    check_vk_result(err);
                    err = vkCreateSemaphore(VKDevice::Instance()->GetDevice(), &info, nullptr, &fd->RenderCompleteSemaphore);
                    check_vk_result(err);
                }
            }
            
            auto swapChain = ((VKSwapchain*)VKRenderer::GetRenderer()->GetSwapchain());
            wd->Swapchain = swapChain->GetSwapchain();
            wd->Width = width;
            wd->Height = height;

            wd->BackBufferCount = (uint32_t)swapChain->GetSwapchainBufferCount();
            
			m_Renderpass = new VKRenderpass();
            TextureType textureTypes[1] = { TextureType::COLOUR};
            graphics::api::RenderpassInfo renderpassCI{};
            renderpassCI.attachmentCount = 1;
            renderpassCI.textureType = textureTypes;
            renderpassCI.clear = false;
			m_Renderpass->Init(renderpassCI);
            wd->RenderPass = m_Renderpass->GetRenderpass();

            // Create The Image Views
            {
                for (uint32_t i = 0; i < wd->BackBufferCount; i++)
                {
                    auto scBuffer = swapChain->GetTexture(i);
                    wd->BackBuffer[i] = scBuffer->GetImage();
                    wd->BackBufferView[i] = scBuffer->GetImageView();
                }
            }

			TextureType attachmentTypes[2];
			attachmentTypes[0] = TextureType::COLOUR;

			Texture* attachments[1];
			FramebufferInfo bufferInfo{};
			bufferInfo.width = wd->Width;
			bufferInfo.height = wd->Height;
			bufferInfo.attachmentCount = 1;
			bufferInfo.renderPass = m_Renderpass;
			bufferInfo.attachmentTypes = attachmentTypes;
			bufferInfo.screenFBO = true;

			for (uint32_t i = 0; i < Renderer::GetRenderer()->GetSwapchain()->GetSwapchainBufferCount(); i++)
			{
				attachments[0] = Renderer::GetRenderer()->GetSwapchain()->GetImage(i);
				bufferInfo.attachments = attachments;

				m_Framebuffers[i] = new VKFramebuffer(bufferInfo);
				wd->Framebuffer[i] = m_Framebuffers[i]->GetFramebuffer();
			}
        }

        void VKIMGUIRenderer::Init()
        {
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

            // Upload Fonts
            {
				m_CommandBuffers[wd->FrameIndex]->BeginRecording();

                ImGui_ImplVulkan_CreateFontsTexture(m_CommandBuffers[wd->FrameIndex]->GetCommandBuffer());

				m_CommandBuffers[wd->FrameIndex]->EndRecording();
				m_CommandBuffers[wd->FrameIndex]->Execute(false);

                ImGui_ImplVulkan_InvalidateFontUploadObjects();
            }
        }

        void VKIMGUIRenderer::NewFrame()
        {
        }

        void VKIMGUIRenderer::FrameRender(ImGui_ImplVulkanH_WindowData* wd)
        {
            VkResult err;

            wd->FrameIndex = Renderer::GetRenderer()->GetSwapchain()->GetCurrentBufferId();

            ImGui_ImplVulkanH_FrameData* fd = &wd->Frames[wd->FrameIndex];
            {
				m_CommandBuffers[wd->FrameIndex]->BeginRecording();
            }
            {
				m_Renderpass->BeginRenderpass(m_CommandBuffers[wd->FrameIndex], maths::Vector4(0.0f), m_Framebuffers[wd->FrameIndex], graphics::api::SubPassContents::INLINE, wd->Width, wd->Height);
            }

            // Record Imgui Draw Data and draw funcs into command buffer
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_CommandBuffers[wd->FrameIndex]->GetCommandBuffer());

            // Submit command buffer
			m_Renderpass->EndRenderpass(m_CommandBuffers[wd->FrameIndex]);
            
			//((VKSwapchain*)Renderer::GetRenderer()->GetSwapchain())->AcquireNextImage(fd->ImageAcquiredSemaphore);
            //VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            //VkSubmitInfo info = {};
            //info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            //info.waitSemaphoreCount = 1;
			//VkSemaphore& image_acquired_semaphore = VKRenderer::GetRenderer()->GetPreviousImageAvailable();
			//VkSemaphore& render_finished_semaphore = VKRenderer::GetRenderer()->GetPreviousImageAvailable();
			//info.pWaitSemaphores = &image_acquired_semaphore;
            //info.pWaitDstStageMask = &wait_stage;
            //info.commandBufferCount = 1;
			//auto cmd = m_CommandBuffers[wd->FrameIndex]->GetCommandBuffer();
            //info.pCommandBuffers = &cmd;
            //info.signalSemaphoreCount = 1;
			//info.pSignalSemaphores = &render_finished_semaphore;// fd->RenderCompleteSemaphore;
                
			m_CommandBuffers[wd->FrameIndex]->EndRecording();


            //err = vkQueueSubmit(VKDevice::Instance()->GetGraphicsQueue(), 1, &info, fd->Fence);
            //check_vk_result(err);
			
            VKRenderer::GetRenderer()->Present(m_CommandBuffers[wd->FrameIndex]);
            
			//VKRenderer::GetRenderer()->SetPreviousRenderFinish(render_finished_semaphore);// wd->Frames[wd->FrameIndex].RenderCompleteSemaphore);
        }

        void VKIMGUIRenderer::Render(Lumos::graphics::api::CommandBuffer* commandBuffer)
        {
            FrameRender(&g_WindowData);
        }

        void VKIMGUIRenderer::OnResize(uint width, uint height)
        {
            auto* wd = &g_WindowData;
            auto swapChain = ((VKSwapchain*)VKRenderer::GetRenderer()->GetSwapchain());
            wd->Swapchain = swapChain->GetSwapchain();
            for (uint32_t i = 0; i < wd->BackBufferCount; i++)
            {
                auto scBuffer = swapChain->GetTexture(i);
                wd->BackBuffer[i] = scBuffer->GetImage();
                wd->BackBufferView[i] = scBuffer->GetImageView();
            }

            wd->Width = width;
            wd->Height = height;

            for (uint32_t i = 0; i < wd->BackBufferCount; i++)
            {
                vkDestroyFramebuffer(VKDevice::Instance()->GetDevice(), wd->Framebuffer[i], VK_NULL_HANDLE);
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
                    vkCreateFramebuffer(VKDevice::Instance()->GetDevice(), &info, NULL, &wd->Framebuffer[i]);
                }
            }
        }
    }
}

