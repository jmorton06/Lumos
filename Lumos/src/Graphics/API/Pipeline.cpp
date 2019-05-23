#include "LM.h"
#include "Pipeline.h"
#include "GraphicsContext.h"

#ifdef LUMOS_RENDER_API_VULKAN
#include "Platform/Vulkan/VKPipeline.h"
#endif

#ifdef LUMOS_RENDER_API_OPENGL
#include "Platform/OpenGL/GLPipeline.h"
#endif

namespace lumos
{
	namespace graphics
	{
		Pipeline* Pipeline::Create(const PipelineInfo& pipelineInfo)
		{
			switch (GraphicsContext::GetRenderAPI())
			{
#ifdef LUMOS_RENDER_API_OPENGL
				case RenderAPI::OPENGL: return new GLPipeline(pipelineInfo);
#endif
#ifdef LUMOS_RENDER_API_VULKAN
				case RenderAPI::VULKAN: return new VKPipeline(pipelineInfo);
#endif
			}
			return nullptr;
		}
	}
}
