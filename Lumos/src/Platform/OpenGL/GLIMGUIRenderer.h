#pragma once

#include "LM.h"
#include "Graphics/API/IMGUIRenderer.h"

namespace lumos
{
	namespace graphics
	{
		class GLIMGUIRenderer : public IMGUIRenderer
		{
        public:
            GLIMGUIRenderer(uint width, uint height, bool clearScreen);
            ~GLIMGUIRenderer();

            void Init() override;
            void NewFrame() override;
            void Render(lumos::graphics::CommandBuffer* commandBuffer) override;
            void OnResize(uint width, uint height) override;

		private:
			void* m_WindowHandle;
			bool m_ClearScreen;
        };
    }
}
