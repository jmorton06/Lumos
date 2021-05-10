#pragma once
#include "VKCommandBuffer.h"
#include "VKRenderpass.h"
#include "VKFramebuffer.h"
#include "Graphics/API/Swapchain.h"

#define MAX_SWAPCHAIN_BUFFERS 3
namespace Lumos
{
    class Window;
    namespace Graphics
    {
        class VKFence;

        struct FrameData
        {
            VkSemaphore PresentSemaphore = VK_NULL_HANDLE, RenderSemaphore = VK_NULL_HANDLE;

            Ref<VKFence> RenderFence;
            Ref<VKCommandPool> CommandPool;
            Ref<VKCommandBuffer> MainCommandBuffer;
        };

        class Texture2D;
        class VKSwapchain : public Swapchain
        {
        public:
            VKSwapchain(uint32_t width, uint32_t height);
            ~VKSwapchain();

            bool Init(bool vsync) override;
            void Init(bool vsync, Window* windowHandle);
            void CreateFrameData();
            void AcquireNextImage();
            void QueueSubmit();
            void Present();
            void Begin();
            void End();
            void OnResize(uint32_t width, uint32_t height, bool forceResize = false, Window* windowHandle = nullptr);

            VkSurfaceKHR GetSurface() const { return m_Surface; }
            VkSwapchainKHR GetSwapchain() const { return m_SwapChain; }
            uint32_t GetCurrentBufferIndex() const override { return m_AcquireImageIndex; }
            size_t GetSwapchainBufferCount() const override { return m_SwapchainBufferCount; };
            Texture* GetCurrentImage() override { return (Texture*)m_SwapChainBuffers[m_AcquireImageIndex]; };
            Texture* GetImage(uint32_t index) override { return (Texture*)m_SwapChainBuffers[index]; };

            VkSurfaceKHR CreatePlatformSurface(VkInstance vkInstance, Window* window);
            CommandBuffer* GetCurrentCommandBuffer() override;

            FrameData& GetCurrentFrameData();
            VkFormat GetScreenFormat() const { return m_ColourFormat; }

            static void MakeDefault();

        protected:
            static Swapchain* CreateFuncVulkan(uint32_t width, uint32_t height);

        private:
            FrameData m_Frames[MAX_SWAPCHAIN_BUFFERS];

            void FindImageFormatAndColourSpace();

            std::vector<Texture2D*> m_SwapChainBuffers;

            uint32_t m_CurrentBuffer = 0;
            uint32_t m_AcquireImageIndex = 0;
            uint32_t m_Width;
            uint32_t m_Height;
            uint32_t m_QueueNodeIndex = UINT32_MAX;
            uint32_t m_SwapchainBufferCount;
            bool m_VSyncEnabled = false;

            VkSwapchainKHR m_SwapChain;
            VkSwapchainKHR m_OldSwapChain;
            VkSurfaceKHR m_Surface;
            VkFormat m_ColourFormat;
            VkColorSpaceKHR m_ColourSpace;
        };
    }
}
