#include "LM.h"
#include "Renderer.h"

#ifdef LUMOS_RENDER_API_OPENGL
#include "Platform/OpenGL/GLRenderer.h"
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
#include "DirectX/DXRenderer.h"
#endif

#ifdef LUMOS_RENDER_API_VULKAN
#include "Platform/Vulkan/VKRenderer.h"
#endif

#include "GraphicsContext.h"

#include "../Camera/Camera.h"

namespace Lumos
{
	namespace Graphics
	{
		Renderer* Renderer::s_Instance = nullptr;

		void Renderer::Init(u32 width, u32 height)
		{
			switch (Graphics::GraphicsContext::GetRenderAPI())
			{
#ifdef LUMOS_RENDER_API_OPENGL
			case RenderAPI::OPENGL:	s_Instance = lmnew GLRenderer(width, height); break;
#endif

#ifdef LUMOS_RENDER_API_VULKAN
			case RenderAPI::VULKAN: s_Instance = lmnew Graphics::VKRenderer(width, height); break;
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
			case RenderAPI::DIRECT3D: s_Instance = lmnew D3DRenderer(width, height); break;
#endif
			}
			s_Instance->InitInternal();
		}

		void Renderer::Release()
		{
			delete s_Instance;

			s_Instance = nullptr;
		}
	}
}
