#pragma once
#include "LM.h"
#include "App/Window.h"

namespace lumos
{
	namespace graphics
	{

		enum class LUMOS_EXPORT RenderAPI
		{
		#ifdef LUMOS_RENDER_API_OPENGL
			OPENGL,
		#endif

		#ifdef LUMOS_RENDER_API_DIRECT3D
			DIRECT3D,
		#endif

		#ifdef LUMOS_RENDER_API_VULKAN
			VULKAN,
		#endif
		};

		class LUMOS_EXPORT GraphicsContext
		{
		public:
			virtual ~GraphicsContext();

			static void Create(const WindowProperties& properties, void* deviceContext);
			static void Release();

			static RenderAPI GetRenderAPI() { return s_RenderAPI; }
			static void SetRenderAPI(RenderAPI api) { s_RenderAPI = api; }

			virtual void Init() = 0;
			virtual void Present() = 0;

			virtual size_t GetMinUniformBufferOffsetAlignment() const = 0;

			static GraphicsContext* GetContext() { return s_Context; }

		protected:
			
			static GraphicsContext* s_Context;
			static RenderAPI s_RenderAPI;

		};
	}
}



