#ifndef LUMOS_PLATFORM_MACOS
#include "Precompiled.h"
#endif
#include "VKRenderDevice.h"

namespace Lumos::Graphics
{
    void VKRenderDevice::Init()
    {
    }

    void VKRenderDevice::MakeDefault()
    {
        CreateFunc = CreateFuncVulkan;
    }

    RenderDevice* VKRenderDevice::CreateFuncVulkan()
    {
        return new VKRenderDevice();
    }
}
