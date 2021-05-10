#include "Precompiled.h"
#include "VKIMGUIRenderer.h"
#include <imgui/imgui.h>

#define IMGUI_IMPL_VULKAN_NO_PROTOTYPES
#define VK_NO_PROTOTYPES
#include <imgui/backends/imgui_impl_vulkan.h>

#include "VKDevice.h"
#include "VKCommandBuffer.h"
#include "VKRenderer.h"
#include "VKRenderpass.h"
#include "VKTexture.h"

static ImGui_ImplVulkanH_Window g_WindowData;
static VkAllocationCallbacks* g_Allocator = nullptr;
static VkDescriptorPool g_DescriptorPool = VK_NULL_HANDLE;

static void check_vk_result(VkResult err)
{
    if(err == 0)
        return;
    printf("VkResult %d\n", err);
    if(err < 0)
        abort();
}

namespace Lumos
{
    namespace Graphics
    {
        VKIMGUIRenderer::VKIMGUIRenderer(uint32_t width, uint32_t height, bool clearScreen)
            : m_Framebuffers {}
            , m_Renderpass(nullptr)
            , m_FontTexture(nullptr)
        {
            m_WindowHandle = nullptr;
            m_Width = width;
            m_Height = height;
            m_ClearScreen = clearScreen;
        }

        VKIMGUIRenderer::~VKIMGUIRenderer()
        {
            for(int i = 0; i < Renderer::GetRenderer()->GetSwapchain()->GetSwapchainBufferCount(); i++)
            {
                delete m_Framebuffers[i];
            }

            delete m_Renderpass;
            delete m_FontTexture;

            for(int i = 0; i < Renderer::GetRenderer()->GetSwapchain()->GetSwapchainBufferCount(); i++)
            {
                ImGui_ImplVulkanH_Frame* fd = &g_WindowData.Frames[i];

                vkDestroyFence(VKDevice::Get().GetDevice(), fd->Fence, g_Allocator);
                vkDestroyCommandPool(VKDevice::Get().GetDevice(), fd->CommandPool, g_Allocator);
            }

            vkDestroyDescriptorPool(VKDevice::Get().GetDevice(), g_DescriptorPool, nullptr);

            ImGui_ImplVulkan_Shutdown();
        }

        void VKIMGUIRenderer::SetupVulkanWindowData(ImGui_ImplVulkanH_Window* wd, VkSurfaceKHR surface, int width, int height)
        {
            // Create Descriptor Pool
            {
                VkDescriptorPoolSize pool_sizes[] = {
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
                VkResult err = vkCreateDescriptorPool(VKDevice::Get().GetDevice(), &pool_info, g_Allocator, &g_DescriptorPool);
                check_vk_result(err);
            }

            wd->Surface = surface;
            wd->ClearEnable = m_ClearScreen;

            auto swapChain = static_cast<VKSwapchain*>(VKRenderer::GetSwapchain());
            wd->Swapchain = swapChain->GetSwapchain();
            wd->Width = width;
            wd->Height = height;

            wd->ImageCount = static_cast<uint32_t>(swapChain->GetSwapchainBufferCount());

            AttachmentInfo textureTypes[2] = {
                { TextureType::COLOUR, TextureFormat::SCREEN }
            };

            Graphics::RenderPassInfo renderpassCI;
            renderpassCI.attachmentCount = 1;
            renderpassCI.textureType = textureTypes;
            renderpassCI.clear = m_ClearScreen;

            m_Renderpass = new VKRenderpass(renderpassCI);
            wd->RenderPass = m_Renderpass->GetHandle();

            wd->Frames = (ImGui_ImplVulkanH_Frame*)IM_ALLOC(sizeof(ImGui_ImplVulkanH_Frame) * wd->ImageCount);
            wd->FrameSemaphores = (ImGui_ImplVulkanH_FrameSemaphores*)IM_ALLOC(sizeof(ImGui_ImplVulkanH_FrameSemaphores) * wd->ImageCount);
            memset(wd->Frames, 0, sizeof(wd->Frames[0]) * wd->ImageCount);
            memset(wd->FrameSemaphores, 0, sizeof(wd->FrameSemaphores[0]) * wd->ImageCount);

            // Create The Image Views
            {
                for(uint32_t i = 0; i < wd->ImageCount; i++)
                {
                    auto scBuffer = (VKTexture2D*)swapChain->GetImage(i);
                    wd->Frames[i].Backbuffer = scBuffer->GetImage();
                    wd->Frames[i].BackbufferView = scBuffer->GetImageView();
                }
            }

            TextureType attachmentTypes[1];
            attachmentTypes[0] = TextureType::COLOUR;

            Texture* attachments[1];
            FramebufferInfo bufferInfo {};
            bufferInfo.width = wd->Width;
            bufferInfo.height = wd->Height;
            bufferInfo.attachmentCount = 1;
            bufferInfo.renderPass = m_Renderpass;
            bufferInfo.attachmentTypes = attachmentTypes;
            bufferInfo.screenFBO = true;

            for(uint32_t i = 0; i < Renderer::GetRenderer()->GetSwapchain()->GetSwapchainBufferCount(); i++)
            {
                attachments[0] = Renderer::GetRenderer()->GetSwapchain()->GetImage(i);
                bufferInfo.attachments = attachments;

                m_Framebuffers[i] = new VKFramebuffer(bufferInfo);
                wd->Frames[i].Framebuffer = m_Framebuffers[i]->GetFramebuffer();
            }
        }

        void VKIMGUIRenderer::Init()
        {
            int w, h;
            w = (int)m_Width;
            h = (int)m_Height;
            ImGui_ImplVulkanH_Window* wd = &g_WindowData;
            VkSurfaceKHR surface = VKContext::Get()->GetSwapchain()->GetSurface();
            SetupVulkanWindowData(wd, surface, w, h);

            // Setup Vulkan binding
            ImGui_ImplVulkan_InitInfo init_info = {};
            init_info.Instance = static_cast<VKContext*>(VKContext::GetContext())->GetVKInstance();
            init_info.PhysicalDevice = VKDevice::Get().GetGPU();
            init_info.Device = VKDevice::Get().GetDevice();
            init_info.QueueFamily = VKDevice::Get().GetPhysicalDevice()->GetGraphicsQueueFamilyIndex();
            init_info.Queue = VKDevice::Get().GetGraphicsQueue();
            init_info.PipelineCache = VKDevice::Get().GetPipelineCache();
            init_info.DescriptorPool = g_DescriptorPool;
            init_info.Allocator = g_Allocator;
            init_info.CheckVkResultFn = NULL;
            init_info.MinImageCount = 2;
            init_info.ImageCount = (uint32_t)Renderer::GetRenderer()->GetSwapchain()->GetSwapchainBufferCount();
            ImGui_ImplVulkan_Init(&init_info, wd->RenderPass);
            // Upload Fonts
            {
                ImGuiIO& io = ImGui::GetIO();

                unsigned char* pixels;
                int width, height;
                io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

                m_FontTexture = new VKTexture2D(width, height, pixels, TextureParameters(TextureFilter::NEAREST, TextureFilter::NEAREST));

                VkWriteDescriptorSet write_desc[1] = {};
                write_desc[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                write_desc[0].dstSet = ImGui_ImplVulkanH_GetFontDescriptor();
                write_desc[0].descriptorCount = 1;
                write_desc[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                write_desc[0].pImageInfo = m_FontTexture->GetDescriptor();
                vkUpdateDescriptorSets(VKDevice::GetHandle(), 1, write_desc, 0, nullptr);

                io.Fonts->TexID = (ImTextureID)m_FontTexture->GetHandle(); // GetImage();

                ImGui_ImplVulkan_AddTexture(io.Fonts->TexID, ImGui_ImplVulkanH_GetFontDescriptor());

                //ImGui_ImplVulkan_InvalidateFontUploadObjects();
            }
        }

        void VKIMGUIRenderer::NewFrame()
        {
        }

        void VKIMGUIRenderer::FrameRender(ImGui_ImplVulkanH_Window* wd)
        {
            wd->FrameIndex = Renderer::GetRenderer()->GetSwapchain()->GetCurrentBufferIndex();

            m_Renderpass->BeginRenderpass(Renderer::GetSwapchain()->GetCurrentCommandBuffer(), Maths::Vector4(0.1f, 0.1f, 0.1f, 1.0f), m_Framebuffers[wd->FrameIndex], Graphics::SubPassContents::INLINE, wd->Width, wd->Height);

            // Record Imgui Draw Data and draw funcs into command buffer
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), ((VKCommandBuffer*)Renderer::GetSwapchain()->GetCurrentCommandBuffer())->GetHandle());

            m_Renderpass->EndRenderpass(Renderer::GetSwapchain()->GetCurrentCommandBuffer());
        }

        void VKIMGUIRenderer::Render(Lumos::Graphics::CommandBuffer* commandBuffer)
        {
            FrameRender(&g_WindowData);
        }

        void VKIMGUIRenderer::OnResize(uint32_t width, uint32_t height)
        {
            auto* wd = &g_WindowData;
            auto swapChain = static_cast<VKSwapchain*>(VKRenderer::GetSwapchain());
            wd->Swapchain = swapChain->GetSwapchain();
            for(uint32_t i = 0; i < wd->ImageCount; i++)
            {
                auto scBuffer = (VKTexture2D*)swapChain->GetImage(i);
                wd->Frames[i].Backbuffer = scBuffer->GetImage();
                wd->Frames[i].BackbufferView = scBuffer->GetImageView();
            }

            wd->Width = width;
            wd->Height = height;

            for(uint32_t i = 0; i < wd->ImageCount; i++)
            {
                delete m_Framebuffers[i];
            }
            // Create Framebuffer
            TextureType attachmentTypes[1];
            attachmentTypes[0] = TextureType::COLOUR;

            Texture* attachments[1];
            FramebufferInfo bufferInfo {};
            bufferInfo.width = wd->Width;
            bufferInfo.height = wd->Height;
            bufferInfo.attachmentCount = 1;
            bufferInfo.renderPass = m_Renderpass;
            bufferInfo.attachmentTypes = attachmentTypes;
            bufferInfo.screenFBO = true;

            for(uint32_t i = 0; i < Renderer::GetRenderer()->GetSwapchain()->GetSwapchainBufferCount(); i++)
            {
                attachments[0] = Renderer::GetRenderer()->GetSwapchain()->GetImage(i);
                bufferInfo.attachments = attachments;

                m_Framebuffers[i] = new VKFramebuffer(bufferInfo);
                wd->Frames[i].Framebuffer = m_Framebuffers[i]->GetFramebuffer();
            }
        }

        void VKIMGUIRenderer::Clear()
        {
            //ImGui_ImplVulkan_ClearDescriptors();
        }

        void VKIMGUIRenderer::MakeDefault()
        {
            CreateFunc = CreateFuncVulkan;
        }

        IMGUIRenderer* VKIMGUIRenderer::CreateFuncVulkan(uint32_t width, uint32_t height, bool clearScreen)
        {
            return new VKIMGUIRenderer(width, height, clearScreen);
        }

        void VKIMGUIRenderer::RebuildFontTexture()
        {
            // Upload Fonts
            {
                ImGuiIO& io = ImGui::GetIO();

                unsigned char* pixels;
                int width, height;
                io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

                m_FontTexture = new VKTexture2D(width, height, pixels, TextureParameters(TextureFilter::NEAREST, TextureFilter::NEAREST, TextureWrap::REPEAT));

                VkWriteDescriptorSet write_desc[1] = {};
                write_desc[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                write_desc[0].dstSet = ImGui_ImplVulkanH_GetFontDescriptor();
                write_desc[0].descriptorCount = 1;
                write_desc[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                write_desc[0].pImageInfo = m_FontTexture->GetDescriptor();
                vkUpdateDescriptorSets(VKDevice::GetHandle(), 1, write_desc, 0, nullptr);

                io.Fonts->TexID = (ImTextureID)m_FontTexture->GetHandle(); // GetImage();

                ImGui_ImplVulkan_AddTexture(io.Fonts->TexID, ImGui_ImplVulkanH_GetFontDescriptor());

                //ImGui_ImplVulkan_InvalidateFontUploadObjects();
            }
        }
    }
}
