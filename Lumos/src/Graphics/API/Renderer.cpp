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
        Renderer*(*Renderer::CreateFunc)(u32, u32) = nullptr;

		Renderer* Renderer::s_Instance = nullptr;

		void Renderer::Init(u32 width, u32 height)
		{
            LUMOS_CORE_ASSERT(CreateFunc, "No Renderer Create Function");
            
            s_Instance = CreateFunc(width, height);
			s_Instance->InitInternal();
		}

		void Renderer::Release()
		{
			delete s_Instance;

			s_Instance = nullptr;
		}
	}
}
