#include "Precompiled.h"
#include "RenderDevice.h"

namespace Lumos
{
    namespace Graphics
    {
        RenderDevice* (*RenderDevice::CreateFunc)() = nullptr;

        RenderDevice* RenderDevice::s_Instance = nullptr;

        void RenderDevice::Create()
        {
            LUMOS_ASSERT(CreateFunc, "No RenderDevice Create Function");

            s_Instance = CreateFunc();
        }

        void RenderDevice::Release()
        {
            delete s_Instance;
        };
    }
}
