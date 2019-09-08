#include "LM.h"
#include "DescriptorSet.h"
#include "GraphicsContext.h"

#ifdef LUMOS_RENDER_API_VULKAN 
#include "Platform/Vulkan/VKDescriptorSet.h"
#endif

#ifdef LUMOS_RENDER_API_OPENGL
#include "Platform/OpenGL/GLDescriptorSet.h"
#endif

namespace Lumos
{
	namespace Graphics
	{
        DescriptorSet*(*DescriptorSet::CreateFunc)(const DescriptorInfo&) = nullptr;

		DescriptorSet* DescriptorSet::Create(const DescriptorInfo& info)
		{
            LUMOS_ASSERT(CreateFunc, "No DescriptorSet Create Function");
            
            return CreateFunc(info);
		}
	}
}
