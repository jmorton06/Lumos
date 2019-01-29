#pragma once
#include "VK.h"
#include "Graphics/API/Pipeline.h"
#include "VKDescriptorSet.h"

namespace Lumos
{
	class Shader;

	namespace graphics
	{
		class VKCommandBuffer;

		class VKPipeline : public api::Pipeline
		{
		public:
			VKPipeline();
			VKPipeline(const api::PipelineInfo& pipelineCI);
			~VKPipeline();

			bool Init(const api::PipelineInfo& pipelineCI);

			void Unload() const;
			void SetActive(graphics::api::CommandBuffer* cmdBuffer) override;

			VkDescriptorSetLayout* GetDescriptorLayout(int id) { return &m_DescriptorLayouts[id]; };

			VkDescriptorPool GetDescriptorPool() const { return m_DescriptorPool; };
			VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; };
			std::string 	 GetPipelineName() 	 const { return m_PipelineName; };
			VkPipeline 		 GetPipeline() 		 const { return m_Pipeline; }

			VkDescriptorSet CreateDescriptorSet();

		private:
			VkVertexInputBindingDescription 	m_VertexBindingDescription;
			VkPipelineLayout 					m_PipelineLayout;
			VkDescriptorPool 					m_DescriptorPool;
			VkPipeline 							m_Pipeline;

			std::vector<VkDescriptorSetLayout> 	m_DescriptorLayouts;

			std::string 						m_PipelineName;
		};
	}
}


