#include "LM.h"
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
        return lmnew VKRenderDevice();
    }
}
