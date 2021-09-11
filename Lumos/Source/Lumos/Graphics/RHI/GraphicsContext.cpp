#include "Precompiled.h"
#include "GraphicsContext.h"

#ifdef LUMOS_RENDER_API_OPENGL
#include "Platform/OpenGL/GLContext.h"
#include "Platform/OpenGL/GLFunctions.h"
#endif
#ifdef LUMOS_RENDER_API_VULKAN
#include "Platform/Vulkan/VKContext.h"
#include "Platform/Vulkan/VKFunctions.h"
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
#include "Graphics/DirectX/DXContext.h"
#include "Graphics/DirectX/DXFunctions.h"
#endif

namespace Lumos
{
    namespace Graphics
    {
        GraphicsContext* (*GraphicsContext::CreateFunc)() = nullptr;

        RenderAPI GraphicsContext::s_RenderAPI;

        GraphicsContext* GraphicsContext::Create()
        {
            LUMOS_ASSERT(CreateFunc, "No GraphicsContext Create Function");
            return CreateFunc();
        }

        GraphicsContext::~GraphicsContext()
        {
        }

        void GraphicsContext::SetRenderAPI(RenderAPI api)
        {
            s_RenderAPI = api;

            switch(s_RenderAPI)
            {
#ifdef LUMOS_RENDER_API_VULKAN
            case RenderAPI::VULKAN:
                Graphics::Vulkan::MakeDefault();
                break;
#endif

#ifdef LUMOS_RENDER_API_OPENGL
            case RenderAPI::OPENGL:
                Graphics::GL::MakeDefault();
                break;
#endif

#ifdef LUMOS_RENDER_API_DIRECT3D
            case RenderAPI::DIRECT3D:
                Graphics::DIRECT3D::MakeDefault();
                break;
#endif
            default:
                break;
            }
        }
    }
}
