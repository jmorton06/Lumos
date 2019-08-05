#include "LM.h"
#include "IMGUIRenderer.h"
#include "GraphicsContext.h"

#ifdef LUMOS_IMGUI
#ifdef LUMOS_RENDER_API_OPENGL
#include "Platform/OpenGL/GLIMGUIRenderer.h"
#endif

#ifdef LUMOS_RENDER_API_VULKAN
#include "Platform/Vulkan/VKIMGUIRenderer.h"
#endif
#endif

namespace Lumos
{
    namespace Graphics
    {
        IMGUIRenderer* IMGUIRenderer::Create(u32 width, u32 height, bool clearScreen)
		{
#ifdef LUMOS_IMGUI
			switch (GraphicsContext::GetRenderAPI())
			{
#ifdef LUMOS_RENDER_API_OPENGL
                case RenderAPI::OPENGL: return lmnew GLIMGUIRenderer(width, height, clearScreen);
#endif
#ifdef LUMOS_RENDER_API_VULKAN
				case RenderAPI::VULKAN: return lmnew VKIMGUIRenderer(width, height, clearScreen);
#endif
			}
#endif

			return nullptr;
		}
    }
}
