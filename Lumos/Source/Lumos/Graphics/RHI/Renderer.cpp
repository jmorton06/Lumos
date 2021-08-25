#include "Precompiled.h"
#include "Renderer.h"

namespace Lumos
{
    namespace Graphics
    {
        Renderer* (*Renderer::CreateFunc)() = nullptr;

        Renderer* Renderer::s_Instance = nullptr;

        void Renderer::Init()
        {
            LUMOS_ASSERT(CreateFunc, "No Renderer Create Function");
            LUMOS_PROFILE_FUNCTION();
            s_Instance = CreateFunc();
            s_Instance->InitInternal();
        }

        void Renderer::Release()
        {
            delete s_Instance;

            s_Instance = nullptr;
        }
    }
}
