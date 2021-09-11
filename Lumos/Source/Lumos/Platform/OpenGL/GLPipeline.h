#pragma once

#include "Graphics/RHI/Pipeline.h"
#include "Graphics/RHI/Framebuffer.h"

namespace Lumos
{
    namespace Graphics
    {
        class GLRenderPass;
        class CommandBuffer;
        class RenderPass;

        class GLPipeline : public Pipeline
        {
        public:
            GLPipeline(const PipelineDesc& pipelineDesc);
            ~GLPipeline();

            bool Init(const PipelineDesc& pipelineDesc);
            void Bind(Graphics::CommandBuffer* commandBuffer, uint32_t layer) override;
            void End(Graphics::CommandBuffer* commandBuffer) override;
            void ClearRenderTargets(CommandBuffer* commandBuffer) override;
            void BindVertexArray();
            void CreateFramebuffers();
            
            Shader* GetShader() const override { return m_Shader; }

            static void MakeDefault();

        protected:
            static Pipeline* CreateFuncGL(const PipelineDesc& pipelineDesc);

        private:
            Shader* m_Shader = nullptr;
            SharedPtr<RenderPass> m_RenderPass;
            std::vector<SharedPtr<Framebuffer>> m_Framebuffers;
            std::string pipelineName;
            bool m_TransparencyEnabled = false;
            uint32_t m_VertexArray = -1;
            BufferLayout m_VertexBufferLayout;
            CullMode m_CullMode;
            BlendMode m_BlendMode;
        };
    }
}
