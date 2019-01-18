#include "LM.h"
#include "Renderer.h"

#ifdef LUMOS_RENDER_API_OPENGL
#include "Platform/GraphicsAPI/OpenGL/GLRenderer.h"
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
#include "DirectX/DXRenderer.h"
#endif

#ifdef LUMOS_RENDER_API_VULKAN
#include "Platform/GraphicsAPI/Vulkan/VKRenderer.h"
#endif

#include "Context.h"

#include "../Camera/Camera.h"

namespace Lumos
{

	Renderer* Renderer::s_Instance = nullptr;

	void Renderer::Init(uint width, uint height)
	{
		switch (graphics::Context::GetRenderAPI())
		{
#ifdef LUMOS_RENDER_API_OPENGL
		case RenderAPI::OPENGL:	s_Instance = new GLRenderer(width, height); break;
#endif

#ifdef LUMOS_RENDER_API_VULKAN
		case RenderAPI::VULKAN: s_Instance = new graphics::VKRenderer(width, height); break;
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
		case RenderAPI::DIRECT3D: s_Instance = new D3DRenderer(width, height); break;
#endif
		}
		s_Instance->InitInternal();
	}

	void Renderer::Release()
	{
		if (s_Instance != nullptr)
			delete s_Instance;

		s_Instance = nullptr;
	}
}
