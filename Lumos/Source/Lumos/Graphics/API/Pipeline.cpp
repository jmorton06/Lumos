#include "Precompiled.h"
#include "Pipeline.h"

#include "Utilities/CombineHash.h"

namespace Lumos
{
    namespace Graphics
    {
        static std::unordered_map<std::size_t, Ref<Pipeline>> m_PipelineCache;

        Pipeline* (*Pipeline::CreateFunc)(const PipelineInfo&) = nullptr;

        Pipeline* Pipeline::Create(const PipelineInfo& pipelineInfo)
        {
            LUMOS_ASSERT(CreateFunc, "No Pipeline Create Function");
            return CreateFunc(pipelineInfo);
        }

        Ref<Pipeline> Pipeline::Get(const PipelineInfo& pipelineInfo)
        {
            size_t hash = 0;
            HashCombine(hash, pipelineInfo.shader.get(), pipelineInfo.cullMode, pipelineInfo.depthBiasEnabled, pipelineInfo.drawType, pipelineInfo.polygonMode, pipelineInfo.transparencyEnabled, pipelineInfo.renderpass.get());

            auto found = m_PipelineCache.find(hash);
            if(found != m_PipelineCache.end() && found->second)
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

        void Pipeline::DeleteUnusedCache()
        {
            for(const auto& [key, value] : m_PipelineCache)
            {
                if(value && value.GetCounter()->GetReferenceCount() == 1)
                    m_PipelineCache[key] = nullptr;
            }
        }
    }
}
