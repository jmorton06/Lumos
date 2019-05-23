#pragma once
#include "LM.h"
#include "Graphics/API/GraphicsContext.h"

namespace lumos
{
	namespace graphics
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

		};
	}
}

