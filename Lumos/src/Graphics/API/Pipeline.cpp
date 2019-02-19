#include "LM.h"
#include "Pipeline.h"
#include "Context.h"

#ifdef LUMOS_RENDER_API_VULKAN
#include "Platform/Vulkan/VKPipeline.h"
#endif

#ifdef LUMOS_RENDER_API_OPENGL
#include "Platform/OpenGL/GLPipeline.h"
#endif

namespace Lumos
{
	namespace graphics
	{
		namespace api
		{
			Pipeline* Pipeline::Create(const PipelineInfo& pipelineInfo)
			{
				switch (Context::GetRenderAPI())
				{
#ifdef LUMOS_RENDER_API_OPENGL
					case OPENGL: return new GLPipeline(pipelineInfo);
#endif
#ifdef LUMOS_RENDER_API_VULKAN
					case VULKAN: return new VKPipeline(pipelineInfo);
#endif
				}
				return nullptr;
			}
		}
	}
}
