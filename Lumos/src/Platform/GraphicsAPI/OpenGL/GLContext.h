#pragma once
#include "JM.h"
#include "Graphics/API/Context.h"

namespace jm
{
	class GLDebug;
	namespace graphics
	{
		class JM_EXPORT GLContext : public Context
		{
		public:
			GLContext(WindowProperties properties, void* deviceContext);
			~GLContext();

			void Present() override;
			void Init() override {};

			inline static GLContext* Get() { return static_cast<GLContext*>(s_Context); }

			size_t GetMinUniformBufferOffsetAlignment() const override { return 1; }

		};
	}
}

