#pragma once

#include "Graphics/API/IMGUIRenderer.h"

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

        private:
			void* m_WindowHandle;
			uint m_Width;
			uint m_Height;
        };
    }
}
