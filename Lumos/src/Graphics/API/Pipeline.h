#pragma once
#include "LM.h"
#include "DescriptorSet.h"

namespace Lumos
{
	class Shader;

	namespace graphics
	{
		namespace api
		{

			class RenderPass;
			class CommandBuffer;

			enum class CullMode
			{
				FRONT,
				BACK,
				FRONTANDBACK,
				NONE
			};

			struct PipelineInfo
			{
				Shader* shader;
				api::RenderPass* vulkanRenderpass;
				api::VertexInputDescription* vertexLayout;

				uint32_t numVertexLayout;
				size_t strideSize;
				api::DescriptorPoolInfo* typeCounts;

				std::vector<DescriptorLayout> descriptorLayouts;
				uint numLayoutBindings;

				api::CullMode cullMode;
				std::string pipelineName;
				int numColorAttachments;
				bool wireframeEnabled;
				bool transparencyEnabled;
				bool depthBiasEnabled;
				uint width;
				uint height;
				uint maxObjects;

			};
			class LUMOS_EXPORT Pipeline
			{
			public:
				static Pipeline* Create(const PipelineInfo& pipelineInfo);
				virtual ~Pipeline() {};

                virtual void SetActive(CommandBuffer* cmdBuffer) = 0;

                DescriptorSet* GetDescriptorSet() const { return descriptorSet; }
				Shader* GetShader() const { return m_Shader; }

            protected:

                DescriptorSet* descriptorSet = nullptr;
				Shader* m_Shader = nullptr;

			};
		}
	}
}
