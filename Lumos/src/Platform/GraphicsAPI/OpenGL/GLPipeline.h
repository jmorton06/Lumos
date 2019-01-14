#pragma once
#include "LM.h"
#include "Graphics/API/Pipeline.h"

namespace Lumos
{
    namespace graphics
    {
        class GLRenderPass;

        namespace api
        {
            class CommandBuffer;
        }

        class GLPipeline : public api::Pipeline
        {
        public:
            GLPipeline();
            GLPipeline(const api::PipelineInfo& pipelineCI);
            ~GLPipeline();

            bool Init(const api::PipelineInfo& pipelineCI);

            void SetActive(graphics::api::CommandBuffer* cmdBuffer) override;

        private:
            GLRenderPass* m_RenderPass;
            std::string pipelineName;
        };
    }
}

