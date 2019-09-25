#include "lmpch.h"
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
        IMGUIRenderer*(*IMGUIRenderer::CreateFunc)(u32, u32, bool) = nullptr;
        
        IMGUIRenderer* IMGUIRenderer::Create(u32 width, u32 height, bool clearScreen)
		{
            LUMOS_ASSERT(CreateFunc, "No IMGUIRenderer Create Function");
            
            return CreateFunc(width, height, clearScreen);
		}
    }
}
