#include "JM.h"
#include "Pipeline.h"
#include "Context.h"

#ifdef JM_RENDER_API_VULKAN
#include "Platform/GraphicsAPI/Vulkan/VKPipeline.h"
#endif

#ifdef JM_RENDER_API_OPENGL
#include "Platform/GraphicsAPI/OpenGL/GLPipeline.h"
#endif

namespace jm
{
	namespace graphics
	{
		namespace api
		{
			Pipeline* Pipeline::Create(const PipelineInfo& pipelineInfo)
			{
				switch (Context::GetRenderAPI())
				{
#ifdef JM_RENDER_API_OPENGL
					case OPENGL: return new GLPipeline(pipelineInfo);
#endif
#ifdef JM_RENDER_API_VULKAN
					case VULKAN: return new VKPipeline(pipelineInfo);
#endif
				}
				return nullptr;
			}
		}
	}
}
