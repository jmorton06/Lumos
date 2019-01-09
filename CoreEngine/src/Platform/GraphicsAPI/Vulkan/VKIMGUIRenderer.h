#pragma once

#include "Graphics/API/IMGUIRenderer.h"

namespace jm
{
	namespace graphics
	{
		class VKIMGUIRenderer : public api::IMGUIRenderer
		{
        public:
            VKIMGUIRenderer(uint width, uint height, void* windowHandle);
            ~VKIMGUIRenderer();

            void Init() override;
            void NewFrame() override;
            void Render(jm::graphics::api::CommandBuffer* commandBuffer) override;

        private:
			void* m_WindowHandle;
			uint m_Width;
			uint m_Height;
        };
    }
}