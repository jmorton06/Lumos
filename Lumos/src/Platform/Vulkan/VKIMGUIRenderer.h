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
            VKIMGUIRenderer(u32 width, u32 height, bool clearScreen);
            ~VKIMGUIRenderer();

            void Init() override;
            void NewFrame() override;
            void Render(Lumos::Graphics::CommandBuffer* commandBuffer) override;
            void OnResize(u32 width, u32 height) override;
			void Clear() override;
            
            void FrameRender(ImGui_ImplVulkanH_WindowData* wd);
            void SetupVulkanWindowData(ImGui_ImplVulkanH_WindowData* wd, VkSurfaceKHR surface, int width, int height);
			bool Implemented() const override { return true; }

        private:
			void* m_WindowHandle;
			u32 m_Width;
			u32 m_Height;
            VKCommandBuffer* m_CommandBuffers[3];
			VKFramebuffer* m_Framebuffers[3];
			VKRenderpass* m_Renderpass;
			VKTexture2D* m_FontTexture;

			bool m_ClearScreen;
            
        };
    }
}
