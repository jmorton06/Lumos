#pragma once

#include "IRenderer.h"
#include "Maths/Maths.h"
#include "Maths/Transform.h"
#include "Graphics/RHI/DescriptorSet.h"

namespace Lumos
{

    class Scene;
    class Camera;

    namespace Graphics
    {
        struct LUMOS_EXPORT LineVertexData
        {
            Maths::Vector3 vertex;
            Maths::Vector4 colour;

            bool operator==(const LineVertexData& other) const
            {
                return vertex == other.vertex && colour == other.colour;
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
        class VertexBuffer;
        class Renderer2D;
        class Texture2D;
        class Material;

        struct LineInfo
        {
            Maths::Vector3 p1;
            Maths::Vector3 p2;
            Maths::Vector4 col;

            LineInfo(const Maths::Vector3& pos1, const Maths::Vector3& pos2, const Maths::Vector4& colour)
            {
                p1 = pos1;
                p2 = pos2;
                col = colour;
            }
        };

        class LUMOS_EXPORT LineRenderer : IRenderer
        {
        public:
            LineRenderer(uint32_t width, uint32_t height, bool clear);
            ~LineRenderer();

            void Init() override;
            void Begin() override;
            void BeginScene(Scene* scene, Camera* overrideCamera, Maths::Transform* overrideCameraTransform) override;
            void Present() override;
            void End() override;
            void EndScene() override {};
            void BeginSimple();

            void RenderInternal();
            void OnResize(uint32_t width, uint32_t height) override;
            void SetScreenBufferSize(uint32_t width, uint32_t height) override;
            void SetRenderTarget(Graphics::Texture* texture, bool rebuildFramebuffer) override;

            void SetSystemUniforms(Graphics::Shader* shader) const;
            float SubmitTexture(Graphics::Texture* texture);
            void PresentToScreen() override;
            void RenderScene() override {};

            void Submit(const Maths::Vector3& p1, const Maths::Vector3& p2, const Maths::Vector4& colour);

            struct UniformBufferObject
            {
                Maths::Matrix4 projView;
            };

            void CreateGraphicsPipeline();
            void CreateFramebuffers();
            void FlushAndResetLines();

            void Clear()
            {
                m_Lines.clear();
            }

        protected:
            void SubmitInternal(const LineInfo& info);

            Graphics::UniformBuffer* m_UniformBuffer = nullptr;
            std::vector<Graphics::CommandBuffer*> m_SecondaryCommandBuffers;
            std::vector<Graphics::VertexBuffer*> m_VertexBuffers;
            Graphics::IndexBuffer* m_IndexBuffer {};

            LineVertexData* m_Buffer = nullptr;
            std::vector<LineInfo> m_Lines;

            uint32_t m_CurrentBufferID = 0;
            uint32_t m_BatchDrawCallIndex = 0;
            uint32_t LineIndexCount = 0;
            bool m_Clear = false;
        };
    }
}
