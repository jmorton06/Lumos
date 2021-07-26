#include "Precompiled.h"
#include "Renderer.h"

namespace Lumos
{
    namespace Graphics
    {
        Renderer* (*Renderer::CreateFunc)(uint32_t, uint32_t) = nullptr;

        Renderer* Renderer::s_Instance = nullptr;

        void Renderer::Init(uint32_t width, uint32_t height)
        {
            LUMOS_ASSERT(CreateFunc, "No Renderer Create Function");
            LUMOS_PROFILE_FUNCTION();
            s_Instance = CreateFunc(width, height);
            s_Instance->InitInternal();
        }

        void Renderer::Release()
        {
            delete s_Instance;

            s_Instance = nullptr;
        }
    }
}
