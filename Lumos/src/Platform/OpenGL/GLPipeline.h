#pragma once
#include "LM.h"
#include "Graphics/API/Pipeline.h"

namespace lumos
{
    namespace graphics
    {
        class GLRenderPass;
        class CommandBuffer;

        class GLPipeline : public Pipeline
        {
        public:
            GLPipeline();
            GLPipeline(const PipelineInfo& pipelineCI);
            ~GLPipeline();

            bool Init(const PipelineInfo& pipelineCI);

            void SetActive(graphics::CommandBuffer* cmdBuffer) override;

        private:
            GLRenderPass* m_RenderPass;
            std::string pipelineName;
        };
    }
}

