#pragma once
#include "lmpch.h"
#include "Renderer.h"

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

		enum class PolygonMode
		{
			Fill,
			Line,
			Point
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
			std::string pipelineName;
			int numColorAttachments;
			PolygonMode polygonMode;
			bool transparencyEnabled;
			bool depthBiasEnabled;
			u32 maxObjects;
			float lineWidth = -1.0f;
			DrawType drawType = DrawType::TRIANGLE;
		};
		class LUMOS_EXPORT Pipeline
		{
		public:
			static Pipeline* Create(const PipelineInfo& pipelineInfo);
			virtual ~Pipeline(){};

			virtual void Bind(CommandBuffer* cmdBuffer) = 0;

			virtual DescriptorSet* GetDescriptorSet() const = 0;
			virtual Shader* GetShader() const = 0;

		protected:
			static Pipeline* (*CreateFunc)(const PipelineInfo&);
		};
	}
}
