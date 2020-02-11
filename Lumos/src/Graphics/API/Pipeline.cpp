#include "lmpch.h"
#include "Pipeline.h"

namespace Lumos
{
	namespace Graphics
	{
        Pipeline*(*Pipeline::CreateFunc)(const PipelineInfo&) = nullptr;

		Pipeline* Pipeline::Create(const PipelineInfo& pipelineInfo)
		{
            LUMOS_ASSERT(CreateFunc, "No Pipeline Create Function");
            
            return CreateFunc(pipelineInfo);
		}
	}
}
