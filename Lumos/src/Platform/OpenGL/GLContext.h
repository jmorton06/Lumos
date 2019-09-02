#pragma once
#include "LM.h"
#include "Graphics/API/GraphicsContext.h"

namespace Lumos
{
	namespace Graphics
	{
		class LUMOS_EXPORT GLContext : public GraphicsContext
		{
		public:
			GLContext(const WindowProperties& properties, void* deviceContext);
			~GLContext();

			void Present() override;
			void Init() override {};

			inline static GLContext* Get() { return static_cast<GLContext*>(s_Context); }

			size_t GetMinUniformBufferOffsetAlignment() const override { return 1; }

            bool FlipImGUITexture() const override { return true; }

			void OnImGUI() override;
            static void MakeDefault();
        protected:
            static GraphicsContext* CreateFuncGL(const WindowProperties& properties, void* cont);
		};
	}
}

