#include "LM.h"
#include "VKRenderDevice.h"

namespace Lumos::Graphics
{
    void VKRenderDevice::Init()
    {
        
    }
    
    void VKCommandBuffer::MakeDefault()
    {
        CreateFunc = CreateFuncVulkan;
    }
    
    CommandBuffer* VKCommandBuffer::CreateFuncVulkan()
    {
        return lmnew VKCommandBuffer();
    }
}
