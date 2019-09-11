#include "lmpch.h"
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
		return lmnew GLRenderDevice();
	}
}
