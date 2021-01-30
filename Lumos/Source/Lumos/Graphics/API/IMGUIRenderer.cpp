#include "Precompiled.h"
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
        IMGUIRenderer*(*IMGUIRenderer::CreateFunc)(uint32_t, uint32_t, bool) = nullptr;
        
        IMGUIRenderer* IMGUIRenderer::Create(uint32_t width, uint32_t height, bool clearScreen)
		{
            LUMOS_ASSERT(CreateFunc, "No IMGUIRenderer Create Function");
            
            return CreateFunc(width, height, clearScreen);
		}
    }
}
