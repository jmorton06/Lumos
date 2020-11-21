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
			FILL,
			LINE,
			POINT
		};

		struct PipelineInfo
		{
            Ref<RenderPass> renderpass;
            //Shader Information
			Ref<Shader> shader;
            BufferLayout vertexBufferLayout;
			std::vector<DescriptorLayoutInfo> descriptorLayouts;
            int numPushConst = 0;
            int pushConstSize = 0;
            //
            
			CullMode cullMode = CullMode::BACK;
			PolygonMode polygonMode = PolygonMode::FILL;
            DrawType drawType = DrawType::TRIANGLE;

			bool transparencyEnabled;
			bool depthBiasEnabled;
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
