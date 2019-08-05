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
        RenderDevice* RenderDevice::s_Instance = nullptr;
        
        void RenderDevice::Create()
        {
            switch (Graphics::GraphicsContext::GetRenderAPI())
            {
#ifdef LUMOS_RENDER_API_OPENGL
                case RenderAPI::OPENGL : s_Instance = lmnew GLRenderDevice();
#endif
#ifdef LUMOS_RENDER_API_VULKAN
                case RenderAPI::VULKAN : s_Instance = lmnew VKRenderDevice();
#endif
            }
        }
        
        void RenderDevice::Release()
        {
            lmdel s_Instance;
        };
    }
}
