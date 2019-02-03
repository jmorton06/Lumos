#pragma once

#include "Graphics/API/IMGUIRenderer.h"
#include "VK.h"

#include "VKCommandBuffer.h"

struct ImGui_ImplVulkanH_WindowData;
namespace Lumos
{
	namespace graphics
	{
		class VKIMGUIRenderer : public api::IMGUIRenderer
		{
        public:
            VKIMGUIRenderer(uint width, uint height);
            ~VKIMGUIRenderer();

            void Init() override;
            void NewFrame() override;
            void Render(Lumos::graphics::api::CommandBuffer* commandBuffer) override;
            void OnResize(uint width, uint height) override;
            
            void FrameRender(ImGui_ImplVulkanH_WindowData* wd);
            void SetupVulkanWindowData(ImGui_ImplVulkanH_WindowData* wd, VkSurfaceKHR surface, int width, int height);


        private:
			void* m_WindowHandle;
			uint m_Width;
			uint m_Height;
            VKCommandBuffer* m_CommandBuffers[3];
            
        };
    }
}
