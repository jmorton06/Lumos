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

        struct PipelineDesc
        {
            SharedRef<RenderPass> renderpass;
            SharedRef<Shader> shader;

            CullMode cullMode = CullMode::BACK;
            PolygonMode polygonMode = PolygonMode::FILL;
            DrawType drawType = DrawType::TRIANGLE;

            bool transparencyEnabled;
            bool depthBiasEnabled = false;
        };

        class LUMOS_EXPORT Pipeline
        {
        public:
            static Pipeline* Create(const PipelineDesc& pipelineInfo);
            static SharedRef<Pipeline> Get(const PipelineDesc& pipelineInfo);
            static void ClearCache();
            static void DeleteUnusedCache();

            virtual ~Pipeline() = default;

            virtual void Bind(CommandBuffer* cmdBuffer) = 0;

            virtual Shader* GetShader() const = 0;

        protected:
            static Pipeline* (*CreateFunc)(const PipelineDesc&);
        };
    }
}
