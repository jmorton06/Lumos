#pragma once

#include "Graphics/RHI/Pipeline.h"

namespace Lumos
{
    namespace Graphics
    {
        class GLRenderPass;
        class CommandBuffer;

        class GLPipeline : public Pipeline
        {
        public:
            GLPipeline(const PipelineDesc& pipelineCreateInfo);
            ~GLPipeline();

            bool Init(const PipelineDesc& pipelineCreateInfo);
            void Bind(Graphics::CommandBuffer* cmdBuffer) override;
            void BindVertexArray();

            Shader* GetShader() const override { return m_Shader; }

            static void MakeDefault();

        protected:
            static Pipeline* CreateFuncGL(const PipelineDesc& pipelineCreateInfo);

        private:
            Shader* m_Shader = nullptr;
            GLRenderPass* m_RenderPass;
            std::string pipelineName;
            bool m_TransparencyEnabled = false;
            uint32_t m_VertexArray = -1;
            BufferLayout m_VertexBufferLayout;
            CullMode m_CullMode;
        };
    }
}
