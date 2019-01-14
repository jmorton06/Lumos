#pragma once

#include "JM.h"
#include "Graphics/API/IMGUIRenderer.h"

namespace jm
{
	namespace graphics
	{
		class GLIMGUIRenderer : public api::IMGUIRenderer
		{
        public:
            GLIMGUIRenderer(uint width, uint height, void* windowHandle);
            ~GLIMGUIRenderer();

            void Init() override;
            void NewFrame() override;
            void Render(jm::graphics::api::CommandBuffer* commandBuffer) override;

		private:
			void* m_WindowHandle;
        };
    }
}