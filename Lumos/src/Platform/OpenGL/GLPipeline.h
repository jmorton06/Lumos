#pragma once

#include "Graphics/API/Pipeline.h"

namespace Lumos
{
    namespace Graphics
    {
        class GLRenderPass;
        class CommandBuffer;

        class GLPipeline : public Pipeline
        {
        public:
            GLPipeline(const PipelineInfo& pipelineCI);
            ~GLPipeline();

            bool Init(const PipelineInfo& pipelineCI);

            void Bind(Graphics::CommandBuffer* cmdBuffer) override;
            
            void BindVertexArray();
			
			DescriptorSet* GetDescriptorSet() const override { return m_DescriptorSet; }
			Shader* GetShader() const override { return m_Shader; }
            
            size_t GetStride() const override
            {
                return m_StrideSize;
            }
            
            static void MakeDefault();
        protected:
            static Pipeline* CreateFuncGL(const PipelineInfo& pipelineCI);
        private:
			DescriptorSet* m_DescriptorSet = nullptr;
			Shader* m_Shader = nullptr;
            GLRenderPass* m_RenderPass;
            std::string pipelineName;
            bool m_TransparencyEnabled = false;
            u32 m_VertexArray = -1;
            std::vector<VertexInputDescription> m_VertexDescriptions;
            u32 m_StrideSize = 0;
        };
    }
}

