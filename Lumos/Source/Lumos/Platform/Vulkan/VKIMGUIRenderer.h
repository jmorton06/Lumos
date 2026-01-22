#pragma once

#include "Graphics/RHI/IMGUIRenderer.h"
#include "VK.h"

#include "VKCommandBuffer.h"

struct ImGui_ImplVulkanH_Window;
namespace Lumos
{
    namespace Graphics
    {
        class VKFramebuffer;
        class VKRenderPass;
        class VKTexture2D;
        class VKCommandBuffer;

        class VKIMGUIRenderer : public IMGUIRenderer
        {
        public:
            VKIMGUIRenderer(uint32_t width, uint32_t height, bool clearScreen);
            ~VKIMGUIRenderer();

            void Init() override;
            void NewFrame() override;
            void Render(Lumos::Graphics::CommandBuffer* commandBuffer) override;
            void OnResize(uint32_t width, uint32_t height) override;
            void Clear() override;

            void FrameRender(ImGui_ImplVulkanH_Window* wd, VKCommandBuffer* commandBuffer);
            void SetupVulkanWindowData(ImGui_ImplVulkanH_Window* wd, VkSurfaceKHR surface, int width, int height);
            bool Implemented() const override { return true; }
            void RebuildFontTexture() override;
            ImTextureID AddTexture(Texture* texture, TextureType type, uint32_t level, uint32_t mip) override;

            static void MakeDefault();

        protected:
            static IMGUIRenderer* CreateFuncVulkan(uint32_t width, uint32_t height, bool clearScreen);

        private:
            void* m_WindowHandle;
            uint32_t m_Width;
            uint32_t m_Height;
            VKFramebuffer* m_Framebuffers[3];
            VKRenderPass* m_Renderpass;
            VKTexture2D* m_FontTexture;
            ImTextureID m_FontTextureID;
            bool m_ClearScreen;

            TDArray<Texture*> m_CurrentFrameTextures;
        };
    }
}
