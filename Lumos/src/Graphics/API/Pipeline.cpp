#include "Precompiled.h"
#include "Pipeline.h"

#include "Utilities/CombineHash.h"

namespace Lumos
{
	namespace Graphics
	{
        static std::unordered_map<std::size_t, Ref<Pipeline>> m_PipelineCache;

        Pipeline*(*Pipeline::CreateFunc)(const PipelineInfo&) = nullptr;

		Pipeline* Pipeline::Create(const PipelineInfo& pipelineInfo)
		{
            LUMOS_ASSERT(CreateFunc, "No Pipeline Create Function");
            return CreateFunc(pipelineInfo);
		}
    
        Ref<Pipeline> Pipeline::Get(const PipelineInfo& pipelineInfo)
        {
            size_t hash = 0;
            HashCombine(hash, pipelineInfo.shader.get(), pipelineInfo.cullMode, pipelineInfo.depthBiasEnabled, pipelineInfo.drawType, pipelineInfo.polygonMode,  pipelineInfo.transparencyEnabled, pipelineInfo.renderpass.get());
            
            const auto& vertexLayout = pipelineInfo.vertexBufferLayout.GetLayout();
            HashCombine(hash, pipelineInfo.vertexBufferLayout.GetStride(), vertexLayout.size() );
            
            for(auto& layout : vertexLayout)
            {
                HashCombine(hash, layout.name, layout.format, layout.normalized, layout.offset);
            }
            
            auto found = m_PipelineCache.find(hash);
            if (found != m_PipelineCache.end())
            {
                return found->second;
            }
            
            auto pipeline = Ref<Pipeline>(Create(pipelineInfo));
            m_PipelineCache[hash] = pipeline;
            return pipeline;
        }
    
        void Pipeline::ClearCache()
        {
            m_PipelineCache.clear();
        }
	}
}
