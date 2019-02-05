#include "LM.h"
#include "IMGUIRenderer.h"
#include "Context.h"

#ifdef LUMOS_IMGUI
#ifdef LUMOS_RENDER_API_OPENGL
#include "Platform/GraphicsAPI/OpenGL/GLIMGUIRenderer.h"
#endif

#ifdef LUMOS_RENDER_API_VULKAN
#include "Platform/GraphicsAPI/Vulkan/VKIMGUIRenderer.h"
#endif
#endif

namespace Lumos
{
    namespace graphics
    {
        namespace api
        {
            IMGUIRenderer* IMGUIRenderer::Create(uint width, uint height, bool clearScreen)
			{
#ifdef LUMOS_IMGUI
				switch (Context::GetRenderAPI())
				{
#ifdef LUMOS_RENDER_API_OPENGL
                    case OPENGL: return new GLIMGUIRenderer(width, height);
#endif
#ifdef LUMOS_RENDER_API_VULKAN
					case VULKAN: return new VKIMGUIRenderer(width, height, clearScreen);
#endif
				}
#endif

				return nullptr;
			}
        }
    }
}
