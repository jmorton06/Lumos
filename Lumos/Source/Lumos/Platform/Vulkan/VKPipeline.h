#pragma once
#include "VK.h"
#include "Graphics/API/Pipeline.h"
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
            VKPipeline(const PipelineInfo& pipelineCreateInfo);
            ~VKPipeline();

            bool Init(const PipelineInfo& pipelineCreateInfo);
            void Bind(CommandBuffer* cmdBuffer) override;

            VkDescriptorSet CreateDescriptorSet();
            VkDescriptorSetLayout* GetDescriptorLayout(int id)
            {
                return &m_DescriptorLayouts[id];
            };

            const VkDescriptorPool& GetDescriptorPool() const
            {
                return m_DescriptorPool;
            };
            const VkPipelineLayout& GetPipelineLayout() const
            {
                return m_PipelineLayout;
            };
            const VkPipeline& GetPipeline() const
            {
                return m_Pipeline;
            }

            DescriptorSet* GetDescriptorSet() const override
            {
                return m_DescriptorSet;
            }
            Shader* GetShader() const override
            {
                return m_Shader.get();
            }

            static void MakeDefault();

        protected:
            static Pipeline* CreateFuncVulkan(const PipelineInfo& pipelineCreateInfo);

        private:
            VkVertexInputBindingDescription m_VertexBindingDescription;
            std::vector<VkDescriptorSetLayout> m_DescriptorLayouts;
            VkDescriptorPool m_DescriptorPool;
            DescriptorSet* m_DescriptorSet = nullptr;
            Ref<Shader> m_Shader;

            VkPipelineLayout m_PipelineLayout;
            VkPipeline m_Pipeline;
        };
    }
}
