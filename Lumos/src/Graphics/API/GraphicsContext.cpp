#include "LM.h"
#include "GraphicsContext.h"

#ifdef LUMOS_RENDER_API_OPENGL
#include "Platform/OpenGL/GLContext.h"
#endif
#ifdef LUMOS_RENDER_API_VULKAN
#include "Platform/Vulkan/VKContext.h"
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
#include "graphics/DirectX/DXContext.h"
#endif

namespace lumos
{
	namespace graphics
	{
		GraphicsContext* GraphicsContext::s_Context = nullptr;
		RenderAPI GraphicsContext::s_RenderAPI;

		void GraphicsContext::Create(const WindowProperties& properties, void* deviceContext)
		{
			switch (GetRenderAPI())
			{
#ifdef LUMOS_RENDER_API_OPENGL
			case RenderAPI::OPENGL:	s_Context = new GLContext(properties, deviceContext); break;
#endif
#ifdef LUMOS_RENDER_API_VULKAN
			case RenderAPI::VULKAN: s_Context = new VKContext(properties, deviceContext); break;
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
			case RenderAPI::DIRECT3D: s_Context = new D3DContext(properties, deviceContext); break;
#endif
			}
		}

		void GraphicsContext::Release()
		{
			delete s_Context;
		}

		GraphicsContext::~GraphicsContext()
		{
		}
	}
}
