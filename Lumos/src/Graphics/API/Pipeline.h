#pragma once
#include "Renderer.h"
#include "BufferLayout.h"

namespace Lumos
{
	namespace Graphics
	{
		class Shader;
		class RenderPass;
		class CommandBuffer;
		class DescriptorSet;
		struct VertexInputDescription;
		struct DescriptorLayoutInfo;
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
            BufferLayout vertexBufferLayout;
			std::vector<DescriptorLayoutInfo> descriptorLayouts;
			CullMode cullMode;
			std::string pipelineName;
			PolygonMode polygonMode = PolygonMode::Fill;
			bool transparencyEnabled;
			bool depthBiasEnabled;
			u32 maxObjects;
			float lineWidth = -1.0f;
            int numPushConst = 0;
            int pushConstSize = 0;
			DrawType drawType = DrawType::TRIANGLE;
		};
		class LUMOS_EXPORT Pipeline
		{
		public:
			static Pipeline* Create(const PipelineInfo& pipelineInfo);
			virtual ~Pipeline() = default;

            virtual void Bind(CommandBuffer* cmdBuffer) = 0;

			virtual DescriptorSet* GetDescriptorSet() const = 0;
			virtual Shader* GetShader() const = 0;

		protected:
			static Pipeline* (*CreateFunc)(const PipelineInfo&);
		};
	}
}
