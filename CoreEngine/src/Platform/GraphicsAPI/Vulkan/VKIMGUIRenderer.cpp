#include "JM.h"
#include "VKIMGUIRenderer.h"
#include "external/imgui/imgui.h"

#ifndef JM_PLATFORM_IOS
#include "external/imgui/examples/imgui_impl_glfw.h"
#endif

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

namespace jm
{
    namespace graphics
    {
        VKIMGUIRenderer::VKIMGUIRenderer(uint width, uint height, void* windowHandle)
        {
            m_Implemented = true;
			m_WindowHandle = windowHandle;
			m_Width = width;
			m_Height = height;

            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
        }

        VKIMGUIRenderer::~VKIMGUIRenderer()
        {
            ImGui_ImplVulkan_Shutdown();
#ifndef JM_PLATFORM_IOS
            ImGui_ImplGlfw_Shutdown();
#endif
            ImGui::DestroyContext();
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
            //printf("[vulkan] Selected PresentMode = %d\n", wd->PresentMode);

            // Create SwapChain, RenderPass, Framebuffer, etc.
            ImGui_ImplVulkanH_CreateWindowDataCommandBuffers(VKDevice::Instance()->GetGPU(), VKDevice::Instance()->GetDevice(), VKDevice::Instance()->GetGraphicsQueueFamilyIndex(), wd, g_Allocator);
            //ImGui_ImplVulkanH_CreateWindowDataSwapChainAndFramebuffer(VKDevice::Instance()->GetGPU(), VKDevice::Instance()->GetDevice(), wd, g_Allocator, width, height);
        }

        void VKIMGUIRenderer::Init()
        {
            VKSwapchain* swapChain = (VKSwapchain*)VKRenderer::GetRenderer()->GetSwapchain();

            int w, h;
#ifndef JM_PLATFORM_IOS
            //glfwGetFramebufferSize((GLFWwindow*)Window::Instance()->GetHandle(), &w, &h);
#endif
            w = (int)m_Width;
            h = (int)m_Height;
            //glfwSetFramebufferSizeCallback(window, glfw_resize_callback);
            ImGui_ImplVulkanH_WindowData* wd = &g_WindowData;
            VkSurfaceKHR surface = VKDevice::Instance()->GetSurface();
            SetupVulkanWindowData(wd, surface, w, h);

            ImGuiIO& io = ImGui::GetIO(); (void)io;
            //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
            //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls

            // Setup GLFW binding
#ifndef JM_PLATFORM_IOS
            ImGui_ImplGlfw_InitForVulkan((GLFWwindow*)m_WindowHandle, false);
#endif

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
            ImGui_ImplVulkan_Init(&init_info, ((VKRenderpass*)swapChain->GetRenderPass())->GetRenderpass());//wd->RenderPass);

            // Setup style
            ImGui::StyleColorsDark();
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
            ImGui_ImplVulkan_NewFrame();
#ifndef JM_PLATFORM_IOS
            ImGui_ImplGlfw_NewFrame();
#endif
            ImGui::NewFrame();
        }

        void VKIMGUIRenderer::Render(jm::graphics::api::CommandBuffer* commandBuffer)
        {
            ImGui::Render();
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), ((VKCommandBuffer*)commandBuffer)->GetCommandBuffer());
        }
    }
}

