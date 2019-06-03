#pragma once

#include "LM.h"
#include "Graphics/API/IMGUIRenderer.h"

namespace Lumos
{
	namespace Graphics
	{
		class GLIMGUIRenderer : public IMGUIRenderer
		{
        public:
            GLIMGUIRenderer(uint width, uint height, bool clearScreen);
            ~GLIMGUIRenderer();

            void Init() override;
            void NewFrame() override;
            void Render(Lumos::Graphics::CommandBuffer* commandBuffer) override;
            void OnResize(uint width, uint height) override;
			bool Implemented() const override { return true; }

		private:
			void* m_WindowHandle;
			bool m_ClearScreen;
        };
    }
}
