#include "JM.h"
#include "Context.h"
#include "VertexArray.h"
#include "../Material.h"
#include "Textures/Texture2D.h"

#ifdef JM_RENDER_API_OPENGL
#include "Platform/GraphicsAPI/OpenGL/GLContext.h"
#endif
#ifdef JM_RENDER_API_VULKAN
#include "Platform/GraphicsAPI/Vulkan/VKContext.h"
#endif
#ifdef JM_RENDER_API_DIRECT3D
#include "graphics/DirectX/DXContext.h"
#endif

namespace jm
{
	namespace graphics
	{
		Context* Context::s_Context = nullptr;
		RenderAPI Context::s_RenderAPI;// = RenderAPI::OPENGL;

		void Context::Create(WindowProperties properties, void* deviceContext)
		{
			switch (GetRenderAPI())
			{
#ifdef JM_RENDER_API_OPENGL
			case RenderAPI::OPENGL:	s_Context = new GLContext(properties, deviceContext); break;
#endif
#ifdef JM_RENDER_API_VULKAN
			case RenderAPI::VULKAN: s_Context = new VKContext(properties, deviceContext); break;
#endif
#ifdef JM_RENDER_API_DIRECT3D
			case RenderAPI::DIRECT3D: s_Context = new D3DContext(properties, deviceContext); break;
#endif
			}
		}

		void Context::Release()
		{
			delete s_Context;
		}

		Context::~Context()
		{
		}
	}
}
