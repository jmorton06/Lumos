#include "LM.h"
#include "GraphicsContext.h"

#ifdef LUMOS_RENDER_API_OPENGL
#include "Platform/OpenGL/GLContext.h"
#endif
#ifdef LUMOS_RENDER_API_VULKAN
#include "Platform/Vulkan/VKContext.h"
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
#include "Graphics/DirectX/DXContext.h"
#endif

namespace Lumos
{
	namespace Graphics
	{
        GraphicsContext*(*GraphicsContext::CreateFunc)(const WindowProperties&, void*) = nullptr;

		GraphicsContext* GraphicsContext::s_Context = nullptr;
		RenderAPI GraphicsContext::s_RenderAPI;

		void GraphicsContext::Create(const WindowProperties& properties, void* deviceContext)
		{
            LUMOS_CORE_ASSERT(CreateFunc, "No GraphicsContext Create Function");
            
            return CreateFunc(properties, deviceContext);
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
