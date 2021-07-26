#pragma once
#include "VK.h"
#include "Graphics/RHI/Pipeline.h"
#include "VKDescriptorSet.h"

namespace Lumos
{
    class Shader;

    namespace Graphics
    {
        class VKCommandBuffer;

        class VKPipeline : public Pipeline
        {
        public:
            VKPipeline(const PipelineDesc& pipelineCreateInfo);
            ~VKPipeline();

            bool Init(const PipelineDesc& pipelineCreateInfo);
            void Bind(CommandBuffer* cmdBuffer) override;

            const VkPipelineLayout& GetPipelineLayout() const
            {
                return m_PipelineLayout;
            };
            const VkPipeline& GetPipeline() const
            {
                return m_Pipeline;
            }

            Shader* GetShader() const override
            {
                return m_Shader.get();
            }

            static void MakeDefault();

        protected:
            static Pipeline* CreateFuncVulkan(const PipelineDesc& pipelineCreateInfo);

        private:
            SharedRef<Shader> m_Shader;

            VkPipelineLayout m_PipelineLayout;
            VkPipeline m_Pipeline;
        };
    }
}
