#pragma once
#include "JM.h"
#include "App/Window.h"

enum JM_EXPORT RenderAPI
{
#ifdef JM_RENDER_API_OPENGL
	OPENGL,
#endif

#ifdef JM_RENDER_API_DIRECT3D
	DIRECT3D,
#endif

#ifdef JM_RENDER_API_VULKAN
	VULKAN,
#endif
};

namespace jm
{

	class Material;
	class Texture2D;
	class VertexArray;

	namespace graphics
	{

		class JM_EXPORT Context
		{
		public:
			virtual ~Context();

			static void Create(WindowProperties properties, void* deviceContext);
			static void Release();

			static RenderAPI GetRenderAPI() { return s_RenderAPI; }
			static void SetRenderAPI(RenderAPI api) { s_RenderAPI = api; }

			virtual void Init() = 0;
			virtual void Present() = 0;

			virtual size_t GetMinUniformBufferOffsetAlignment() const = 0;

			static Context* GetContext() { return s_Context; }

		protected:
			
			static Context* s_Context;
			static RenderAPI s_RenderAPI;

		};
	}
}



