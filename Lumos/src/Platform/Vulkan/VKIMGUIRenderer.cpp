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
        VKIMGUIRenderer::VKIMGUIRenderer(uint width, uint height, bool clearScreen): m_CommandBuffers{},
                                                                                     m_Framebuffers{},
                                                                                     m_Renderpass(nullptr)
        {
	        m_Implemented = true;
	        m_WindowHandle = nullptr;
	        m_Width = width;
	        m_Height = height;
	        m_ClearScreen = clearScreen;
        }

        VKIMGUIRenderer::~VKIMGUIRenderer()
        {
			for (int i = 0; i < Renderer::GetRenderer()->GetSwapchain()->GetSwapchainBufferCount(); i++)
			{
				delete m_CommandBuffers[i];
				delete m_Framebuffers[i];
			}
               
			delete m_Renderpass;

            for (int i = 0; i < Renderer::GetRenderer()->GetSwapchain()->GetSwapchainBufferCount(); i++)
            {
                ImGui_ImplVulkanH_FrameData* fd = &g_WindowData.Frames[i];
                vkDestroyFence(VKDevice::Instance()->GetDevice(), fd->Fence, g_Allocator);
                vkDestroyCommandPool(VKDevice::Instance()->GetDevice(), fd->CommandPool, g_Allocator);
            }
            
            vkDestroyDescriptorPool(VKDevice::Instance()->GetDevice(),g_DescriptorPool,nullptr);
            
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
            wd->ClearEnable = m_ClearScreen;
            
            for (int i = 0; i < Renderer::GetRenderer()->GetSwapchain()->GetSwapchainBufferCount(); i++)
            {
                VKCommandBuffer* commandBuffer = new VKCommandBuffer();
                commandBuffer->Init(true);
                m_CommandBuffers[i] = commandBuffer;
            }
            
            auto swapChain = ((VKSwapchain*)VKRenderer::GetRenderer()->GetSwapchain());
            wd->Swapchain = swapChain->GetSwapchain();
            wd->Width = width;
            wd->Height = height;

            wd->BackBufferCount = (uint32_t)swapChain->GetSwapchainBufferCount();
            
			m_Renderpass = new VKRenderpass();
            TextureType textureTypes[1] = { TextureType::COLOUR };
            graphics::api::RenderpassInfo renderpassCI{};
            renderpassCI.attachmentCount = 1;
            renderpassCI.textureType = textureTypes;
            renderpassCI.clear = m_ClearScreen;
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

			TextureType attachmentTypes[1];
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
				ImGuiIO& io = ImGui::GetIO();

				unsigned char* pixels;
				int width, height;
				io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

				auto fontTex = new VKTexture2D(width, height, pixels);

				 vk::WriteDescriptorSet write_desc[1] = {};
				 write_desc[0].dstSet = ImGui_ImplVulkanH_GetFontDescriptor();
				 write_desc[0].descriptorCount = 1;
				 write_desc[0].descriptorType = vk::DescriptorType::eCombinedImageSampler;
				 write_desc[0].pImageInfo = fontTex->GetDescriptor();
				 VKDevice::Instance()->GetDevice().updateDescriptorSets(1, write_desc, 0, nullptr);

				io.Fonts->TexID = (ImTextureID)fontTex->GetHandle();// GetImage();

				ImGui_ImplVulkan_AddTexture(io.Fonts->TexID, ImGui_ImplVulkanH_GetFontDescriptor());

				ImGui_ImplVulkan_InvalidateFontUploadObjects();
			}

          

        }

        void VKIMGUIRenderer::NewFrame()
        {
        }

        void VKIMGUIRenderer::FrameRender(ImGui_ImplVulkanH_WindowData* wd)
        {
            wd->FrameIndex = Renderer::GetRenderer()->GetSwapchain()->GetCurrentBufferId();

            {
				m_CommandBuffers[wd->FrameIndex]->BeginRecording();
            }
            {
				m_Renderpass->BeginRenderpass(m_CommandBuffers[wd->FrameIndex], maths::Vector4(0.1f,0.1f,0.1f,1.0f), m_Framebuffers[wd->FrameIndex], graphics::api::SubPassContents::INLINE, wd->Width, wd->Height);
            }

            // Record Imgui Draw Data and draw funcs into command buffer
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_CommandBuffers[wd->FrameIndex]->GetCommandBuffer());

			m_Renderpass->EndRenderpass(m_CommandBuffers[wd->FrameIndex]);
                
			m_CommandBuffers[wd->FrameIndex]->EndRecording();

            VKRenderer::GetRenderer()->Present(m_CommandBuffers[wd->FrameIndex]);
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
				delete m_Framebuffers[i];
			}
            // Create Framebuffer
			TextureType attachmentTypes[1];
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

        void VKIMGUIRenderer::Clear()
        {
			//ImGui_ImplVulkan_ClearDescriptors();
        }
    }
}
