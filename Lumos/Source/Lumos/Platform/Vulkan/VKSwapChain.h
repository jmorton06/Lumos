#pragma once
#include "VKCommandBuffer.h"
#include "VKRenderPass.h"
#include "VKFramebuffer.h"
#include "VKContext.h"
#include "Graphics/RHI/SwapChain.h"
#include "Core/DataStructures/TDArray.h"

#define MAX_SWAPCHAIN_BUFFERS 3
// Frames the CPU may record ahead of the GPU. Independent of the swapchain image count:
// per-frame data (command buffers, fences, upload buffers) is indexed by GetCurrentBufferIndex(),
// per-image data (framebuffers, present semaphores) by GetCurrentImageIndex().
#define MAX_FRAMES_IN_FLIGHT 2
namespace Lumos
{
    class Window;
    namespace Graphics
    {
        class VKFence;
        class VKSemaphore;

        struct FrameData
        {
            SharedPtr<VKSemaphore> ImageAcquireSemaphore;
            SharedPtr<VKCommandPool> CommandPool;
            SharedPtr<VKCommandBuffer> MainCommandBuffer;
        };

        class Texture2D;
        class VKSwapChain : public SwapChain
        {
        public:
            VKSwapChain(uint32_t width, uint32_t height);
            ~VKSwapChain();

            bool Init(bool vsync) override;
            bool Init(bool vsync, Window* windowHandle) override;
            void CreateFrameData();
            bool AcquireNextImage();
            void QueueSubmit();
            void Present(VkSemaphore semaphore);
            bool Begin();
            void End();
            void OnResize(uint32_t width, uint32_t height, bool forceResize = false, Window* windowHandle = nullptr);

            VkSurfaceKHR GetSurface() const { return m_Surface; }
            VkSwapchainKHR GetSwapChain() const { return m_SwapChain; }
            uint32_t GetCurrentImageIndex() const override { return m_AcquireImageIndex; };

            uint32_t GetCurrentBufferIndex() const override { return m_CurrentBuffer; }
            size_t GetSwapChainBufferCount() const override { return m_SwapChainBufferCount; };
            Texture* GetCurrentImage() override { return (Texture*)m_SwapChainBuffers[m_AcquireImageIndex]; };
            Texture* GetImage(uint32_t index) override { return (Texture*)m_SwapChainBuffers[index]; };

            VkSurfaceKHR CreatePlatformSurface(VkInstance vkInstance, Window* window);
            CommandBuffer* GetCurrentCommandBuffer() override;
            void SetVSync(bool vsync) override { m_VSyncEnabled = vsync; }
            void SetTransparent(bool transparent) override { m_Transparent = transparent; }
            bool IsTransparent() const override { return m_Transparent; }

            uint32_t GetWidth() const { return m_Width; }
            uint32_t GetHeight() const { return m_Height; }
            // Returns true (once) if the swapchain was silently recreated this frame (suboptimal path).
            bool ConsumeWasRecreated() { bool r = m_WasRecreated; m_WasRecreated = false; return r; }

            FrameData& GetCurrentFrameData();
            uint32_t GetFramesInFlight() const { return m_FramesInFlight; }
            // signalled by the frame's submit and waited on by its present: one per image, since an
            // image can be acquired again before the present that waited on a per-frame one is done
            VkSemaphore GetRenderFinishedSemaphore();
            VkFormat GetScreenFormat() const { return m_ColourFormat; }

            static void MakeDefault();

        protected:
            static SwapChain* CreateFuncVulkan(uint32_t width, uint32_t height);

        private:
            FrameData m_Frames[MAX_SWAPCHAIN_BUFFERS];
            Texture2D* m_SwapChainBuffers[MAX_SWAPCHAIN_BUFFERS];
            SharedPtr<VKSemaphore> m_RenderFinished[MAX_SWAPCHAIN_BUFFERS];

            void FindImageFormatAndColourSpace();

            uint32_t m_CurrentBuffer     = 0;
            uint32_t m_AcquireImageIndex = 0;
            uint32_t m_Width;
            uint32_t m_Height;
            uint32_t m_QueueNodeIndex = UINT32_MAX;
            uint32_t m_SwapChainBufferCount;
            uint32_t m_FramesInFlight = MAX_FRAMES_IN_FLIGHT;
            bool m_VSyncEnabled  = false;
            bool m_Transparent   = false;
            bool m_NeedRecreate  = false;
            bool m_WasRecreated  = false;

            VkSwapchainKHR m_SwapChain;
            VkSwapchainKHR m_OldSwapChain;
            VkSurfaceKHR m_Surface;
            VkFormat m_ColourFormat;
            VkColorSpaceKHR m_ColourSpace;
        };
    }
}
