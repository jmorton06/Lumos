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
            GLIMGUIRenderer(u32 width, u32 height, bool clearScreen);
            ~GLIMGUIRenderer();

            void Init() override;
            void NewFrame() override;
            void Render(Lumos::Graphics::CommandBuffer* commandBuffer) override;
            void OnResize(u32 width, u32 height) override;
			bool Implemented() const override { return true; }
            
            static void MakeDefault();
        protected:
            static IMGUIRenderer* CreateFuncGL(u32 width, u32 height, bool clearScreen);
		private:
			void* m_WindowHandle;
			bool m_ClearScreen;
        };
    }
}
