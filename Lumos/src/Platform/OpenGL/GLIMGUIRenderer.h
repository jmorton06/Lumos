#pragma once

#include "LM.h"
#include "Graphics/API/IMGUIRenderer.h"

namespace Lumos
{
	namespace graphics
	{
		class GLIMGUIRenderer : public api::IMGUIRenderer
		{
        public:
            GLIMGUIRenderer(uint width, uint height, bool clearScreen);
            ~GLIMGUIRenderer();

            void Init() override;
            void NewFrame() override;
            void Render(Lumos::graphics::api::CommandBuffer* commandBuffer) override;
            void OnResize(uint width, uint height) override;

		private:
			void* m_WindowHandle;
			bool m_ClearScreen;
        };
    }
}
