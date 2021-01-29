#pragma once

#include "Graphics/API/IMGUIRenderer.h"
#include "VK.h"

#include "VKCommandBuffer.h"

struct ImGui_ImplVulkanH_WindowData;
namespace Lumos
{
	namespace Graphics
	{
		class VKFramebuffer;
		class VKRenderpass;
		class VKTexture2D;

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
            
            void FrameRender(ImGui_ImplVulkanH_WindowData* wd);
            void SetupVulkanWindowData(ImGui_ImplVulkanH_WindowData* wd, VkSurfaceKHR surface, int width, int height);
			bool Implemented() const override { return true; }
            void RebuildFontTexture() override;
            
            static void MakeDefault();
        protected:
            static IMGUIRenderer* CreateFuncVulkan(uint32_t width, uint32_t height, bool clearScreen);
        private:
			void* m_WindowHandle;
			uint32_t m_Width;
			uint32_t m_Height;
            VKCommandBuffer* m_CommandBuffers[3];
			VKFramebuffer* m_Framebuffers[3];
			VKRenderpass* m_Renderpass;
			VKTexture2D* m_FontTexture;

			bool m_ClearScreen;
            
        };
    }
}
