#ifndef LUMOS_PLATFORM_MACOS
#include "Precompiled.h"
#endif
#include "GLRenderDevice.h"

namespace Lumos::Graphics
{
    void GLRenderDevice::Init()
    {
    }

    void GLRenderDevice::MakeDefault()
    {
        CreateFunc = CreateFuncGL;
    }

    RenderDevice* GLRenderDevice::CreateFuncGL()
    {
        return new GLRenderDevice();
    }
}
