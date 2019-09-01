#pragma once
#include "LM.h"
#include "App/Window.h"

namespace Lumos
{
	namespace Graphics
	{

        enum class LUMOS_EXPORT RenderAPI
		{
		#ifdef LUMOS_RENDER_API_OPENGL
			OPENGL = 0,
		#endif

		#ifdef LUMOS_RENDER_API_VULKAN
			VULKAN = 1,
		#endif

		#ifdef LUMOS_RENDER_API_DIRECT3D
			DIRECT3D = 2, //Unsupported
		#endif

		#ifdef LUMOS_RENDER_API_NONE
			METAL = 3, //Unsupported
		#endif

		#ifdef LUMOS_RENDER_API_NONE
			NONE = 4, //Unsupported
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
            virtual bool FlipImGUITexture() const = 0;

			virtual void OnImGUI() = 0;

        protected:
            static GraphicsContext* (*CreateFunc)(const WindowProperties&, void*);

			static GraphicsContext* s_Context;
			static RenderAPI s_RenderAPI;

		};
	}
}



