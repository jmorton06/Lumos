#include "LM.h"
#include "RenderDevice.h"
#include "GraphicsContext.h"

#ifdef LUMOS_RENDER_API_VULKAN
#include "Platform/Vulkan/VKRenderDevice.h"
#endif

#ifdef LUMOS_RENDER_API_OPENGL
#include "Platform/OpenGL/GLRenderDevice.h"
#endif

namespace Lumos
{
    namespace Graphics
    {
        RenderDevice*(*RenderDevice::CreateFunc)() = nullptr;

        RenderDevice* RenderDevice::s_Instance = nullptr;
        
        void RenderDevice::Create()
        {
            LUMOS_CORE_ASSERT(CreateFunc, "No RenderDevice Create Function");
            
			s_Instance = CreateFunc();
        }
        
        void RenderDevice::Release()
        {
            lmdel s_Instance;
        };
    }
}
