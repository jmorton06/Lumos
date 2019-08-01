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
		Pipeline* Pipeline::Create(const PipelineInfo& pipelineInfo)
		{
			switch (GraphicsContext::GetRenderAPI())
			{
#ifdef LUMOS_RENDER_API_OPENGL
				case RenderAPI::OPENGL: return lmnew GLPipeline(pipelineInfo);
#endif
#ifdef LUMOS_RENDER_API_VULKAN
				case RenderAPI::VULKAN: return lmnew VKPipeline(pipelineInfo);
#endif
			}
			return nullptr;
		}
	}
}
