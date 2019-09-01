#include "LM.h"
#include "Pipeline.h"
#include "GraphicsContext.h"

#ifdef LUMOS_RENDER_API_VULKAN
#include "Platform/Vulkan/VKPipeline.h"
#endif

#ifdef LUMOS_RENDER_API_OPENGL
#include "Platform/OpenGL/GLPipeline.h"
#endif

namespace Lumos
{
	namespace Graphics
	{
        Pipeline*(*Pipeline::CreateFunc)(const PipelineInfo&) = nullptr;

		Pipeline* Pipeline::Create(const PipelineInfo& pipelineInfo)
		{
            LUMOS_CORE_ASSERT(CreateFunc, "No Pipeline Create Function");
            
            return CreateFunc(pipelineInfo);
		}
	}
}
