#pragma once

#include "IRenderer.h"
#include "Maths/Maths.h"
#include "Graphics/RHI/DescriptorSet.h"
#include "Maths/Transform.h"

#define RENDERER2DPOINT_VERTEX_SIZE sizeof(PointVertexData)

namespace Lumos
{
    class Scene;
    class Camera;

    namespace Graphics
    {
        struct PointInfo
        {
            Maths::Vector3 p1;
            Maths::Vector4 col;
            float size;

            PointInfo(const Maths::Vector3& pos1, float s, const Maths::Vector4& colour)
            {
                p1 = pos1;
                size = s;
                col = colour;
            }
        };

        struct LUMOS_EXPORT PointVertexData
        {
            Maths::Vector3 vertex;
            Maths::Vector4 colour;
            Maths::Vector2 size;
            Maths::Vector2 uv;

            bool operator==(const PointVertexData& other) const
            {
                return vertex == other.vertex && colour == other.colour && size == other.size && uv == other.uv;
            }
        };

        class RenderPass;
        class Pipeline;
        class DescriptorSet;
        class CommandBuffer;
        class UniformBuffer;
        class Renderable2D;
        class Framebuffer;
        class Texture;
        class Shader;
        class IndexBuffer;

        class LUMOS_EXPORT PointRenderer : public IRenderer
        {
        public:
            PointRenderer(uint32_t width, uint32_t height, bool clear);
            ~PointRenderer();

            void Init() override;
            void Begin() override;
            void BeginScene(Scene* scene, Camera* overrideCamera, Maths::Transform* overrideCameraTransform) override;
            void Present() override;
            void End() override;
            void EndScene() override {};
            void OnResize(uint32_t width, uint32_t height) override;
            void SetScreenBufferSize(uint32_t width, uint32_t height) override;
            void SetRenderTarget(Graphics::Texture* texture, bool rebuildFramebuffer) override;
            void RenderScene() override {};

            void RenderInternal();
            void PresentToScreen() override;
            void Submit(const Maths::Vector3& p1, float size, const Maths::Vector4& colour);
            void SetSystemUniforms(Graphics::Shader* shader) const;
            float SubmitTexture(Graphics::Texture* texture);

            struct UniformBufferObject
            {
                Maths::Matrix4 projView;
            };

            void CreateGraphicsPipeline();
            void CreateFramebuffers();
            void UpdateDesciptorSet() const;

            void Clear()
            {
                m_Points.clear();
            }

        protected:
            void FlushAndResetPoints();
            void SubmitInternal(PointInfo& pointInfo);

            PointVertexData* m_Buffer = nullptr;
            Graphics::IndexBuffer* m_IndexBuffer = nullptr;
            Graphics::UniformBuffer* m_UniformBuffer = nullptr;
            std::vector<Graphics::CommandBuffer*> m_SecondaryCommandBuffers;
            std::vector<Graphics::VertexBuffer*> m_VertexBuffers;
            std::vector<PointInfo> m_Points;

            uint32_t m_BatchDrawCallIndex = 0;
            uint32_t PointIndexCount = 0;
            uint32_t m_IndexCount = 0;
            uint32_t m_CurrentBufferID = 0;
            bool m_Clear = false;
        };
    }
}
