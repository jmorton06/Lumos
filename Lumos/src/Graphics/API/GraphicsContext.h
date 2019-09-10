#pragma once
#include "LM.h"
#include "App/Window.h"

namespace Lumos
{
	namespace Graphics
	{

        enum class RenderAPI : u32
		{
		#ifdef LUMOS_RENDER_API_OPENGL
			OPENGL,
		#endif

		#ifdef LUMOS_RENDER_API_VULKAN
			VULKAN,
		#endif

		#ifdef LUMOS_RENDER_API_DIRECT3D
			DIRECT3D, //Unsupported
		#endif

		#ifdef LUMOS_RENDER_API_NONE
			METAL, //Unsupported
		#endif

		#ifdef LUMOS_RENDER_API_NONE
			NONE, //Unsupported
		#endif
		};

		class LUMOS_EXPORT GraphicsContext
		{
		public:
			virtual ~GraphicsContext();

			static void Create(const WindowProperties& properties, void* deviceContext);
			static void Release();

			static RenderAPI GetRenderAPI() { return s_RenderAPI; }
			static void SetRenderAPI(RenderAPI api);

			virtual void Init() = 0;
			virtual void Present() = 0;

			virtual size_t GetMinUniformBufferOffsetAlignment() const = 0;

			static GraphicsContext* GetContext() { return s_Context; }
            virtual bool FlipImGUITexture() const = 0;

			virtual void OnImGui() = 0;

        protected:
            static GraphicsContext* (*CreateFunc)(const WindowProperties&, void*);

			static GraphicsContext* s_Context;
			static RenderAPI s_RenderAPI;

		};
	}
}



