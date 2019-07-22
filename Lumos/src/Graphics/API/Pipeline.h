#pragma once
#include "LM.h"

namespace Lumos
{
	namespace Graphics
	{
		class Shader;
		class RenderPass;
		class CommandBuffer;
		class DescriptorSet;
		struct VertexInputDescription;
		struct DescriptorLayout;
		struct DescriptorPoolInfo;

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
			RenderPass* renderpass;
			VertexInputDescription* vertexLayout;

			uint32_t numVertexLayout;
			size_t strideSize;
			DescriptorPoolInfo* typeCounts;

			std::vector<DescriptorLayout> descriptorLayouts;
			u32 numLayoutBindings;

			CullMode cullMode;
			String pipelineName;
			int numColorAttachments;
			bool wireframeEnabled;
			bool transparencyEnabled;
			bool depthBiasEnabled;
			u32 width;
			u32 height;
			u32 maxObjects;

		};
		class LUMOS_EXPORT Pipeline
		{
		public:
			static Pipeline* Create(const PipelineInfo& pipelineInfo);
			virtual ~Pipeline() {};

			virtual void SetActive(CommandBuffer* cmdBuffer) = 0;

			virtual DescriptorSet* GetDescriptorSet() const = 0;
			virtual Shader* GetShader() const = 0;
		};
	}
}
