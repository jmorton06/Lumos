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
			VKPipeline(const PipelineInfo& pipelineCI);
			~VKPipeline();

			bool Init(const PipelineInfo& pipelineCI);

			void Unload() const;
            void Bind(CommandBuffer* cmdBuffer) override;

            VkDescriptorSet CreateDescriptorSet();

			VkDescriptorSetLayout* GetDescriptorLayout(int id)
			{
				return &m_DescriptorLayouts[id];
			};

			const std::string& GetPipelineName() const
			{
				return m_PipelineName;
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
				return m_Shader;
			}
            
            size_t GetStride() const override
            {
                return m_VertexBindingDescription.stride;
            }

			static void MakeDefault();

		protected:
			static Pipeline* CreateFuncVulkan(const PipelineInfo& pipelineCI);

		private:
			
			VkVertexInputBindingDescription m_VertexBindingDescription;
			std::vector<VkDescriptorSetLayout> m_DescriptorLayouts;
			VkDescriptorPool m_DescriptorPool;
			DescriptorSet* m_DescriptorSet = nullptr;
			Shader* m_Shader = nullptr;
			
			VkPipelineLayout m_PipelineLayout;
			VkPipeline m_Pipeline;
			float m_LineWidth = -1.0f;
			std::string m_PipelineName;
		};
	}
}
