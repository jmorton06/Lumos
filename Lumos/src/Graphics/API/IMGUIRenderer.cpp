#include "JM.h"
#include "IMGUIRenderer.h"
#include "Context.h"

#ifdef JM_IMGUI
#ifdef JM_RENDER_API_OPENGL
#include "Platform/GraphicsAPI/OpenGL/GLIMGUIRenderer.h"
#endif

#ifdef JM_RENDER_API_VULKAN
#include "Platform/GraphicsAPI/Vulkan/VKIMGUIRenderer.h"
#endif
#endif

namespace jm
{
    namespace graphics
    {
        namespace api
        {
            IMGUIRenderer* IMGUIRenderer::Create(uint width, uint height, void* windowHandle)
			{
#ifdef JM_IMGUI
				switch (Context::GetRenderAPI())
				{
#ifdef JM_RENDER_API_OPENGL
					case OPENGL: return new GLIMGUIRenderer(width, height, windowHandle);
#endif
#ifdef JM_RENDER_API_VULKAN
					case VULKAN: return new VKIMGUIRenderer(width, height, windowHandle);
#endif
				}
#endif

				return nullptr;
			}
        }
    }
}