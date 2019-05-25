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

namespace lumos
{
    namespace graphics
    {
        IMGUIRenderer* IMGUIRenderer::Create(uint width, uint height, bool clearScreen)
		{
#ifdef LUMOS_IMGUI
			switch (GraphicsContext::GetRenderAPI())
			{
#ifdef LUMOS_RENDER_API_OPENGL
                case RenderAPI::OPENGL: return new GLIMGUIRenderer(width, height, clearScreen);
#endif
#ifdef LUMOS_RENDER_API_VULKAN
				case RenderAPI::VULKAN: return new VKIMGUIRenderer(width, height, clearScreen);
#endif
			}
#endif

			return nullptr;
		}
    }
}
