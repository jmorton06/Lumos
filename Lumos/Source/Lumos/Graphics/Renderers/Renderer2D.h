#pragma once

#include "IRenderer.h"
#include "Graphics/Renderable2D.h"
#include "Maths/Maths.h"
#include "Maths/Transform.h"

#define MAX_BOUND_TEXTURES 16
#define MAP_VERTEX_ARRAY 1
namespace Lumos
{
    class Scene;
    class Camera;

    namespace Graphics
    {
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

        struct TriangleInfo
        {
            Maths::Vector3 p1;
            Maths::Vector3 p2;
            Maths::Vector3 p3;
            Maths::Vector4 col;

            TriangleInfo(const Maths::Vector3& pos1, const Maths::Vector3& pos2, const Maths::Vector3& pos3, const Maths::Vector4& colour)
            {
                p1 = pos1;
                p2 = pos2;
                p3 = pos3;
                col = colour;
            }
        };

        struct LUMOS_EXPORT RenderCommand2D
        {
            Renderable2D* renderable = nullptr;
            Maths::Matrix4 transform;
        };

        typedef std::vector<RenderCommand2D> CommandQueue2D;

        struct Render2DLimits
        {
            uint32_t MaxQuads = 10000;
            uint32_t QuadsSize = RENDERER2D_VERTEX_SIZE * 4;
            uint32_t BufferSize = 10000 * RENDERER2D_VERTEX_SIZE * 4;
            uint32_t IndiciesSize = 10000 * 6;
            uint32_t MaxTextures = 16;
            uint32_t MaxBatchDrawCalls = 100;

            void SetMaxQuads(uint32_t quads)
            {
                MaxQuads = quads;
                BufferSize = MaxQuads * RENDERER2D_VERTEX_SIZE * 4;
                IndiciesSize = MaxQuads * 6;
            }
        };

        class LUMOS_EXPORT Renderer2D : public IRenderer
        {
        public:
            Renderer2D(uint32_t width, uint32_t height, bool clear = true, bool triangleIndicies = false, bool renderToDepthTexture = true);
            virtual ~Renderer2D();

            virtual void Init() override;
            virtual void Begin() override;
            virtual void BeginScene(Scene* scene, Camera* overrideCamera, Maths::Transform* overrideCameraTransform) override;
            virtual void Present() override;
            virtual void EndScene() override {};
            virtual void End() override;
            virtual void OnResize(uint32_t width, uint32_t height) override;
            virtual void SetRenderTarget(Texture* texture, bool rebuildFrameBuffer = true) override;
            virtual void RenderScene() override;

            virtual void SubmitTriangle(const Maths::Vector3& p1, const Maths::Vector3& p2, const Maths::Vector3& p3, const Maths::Vector4& colour);
            virtual void Submit(Renderable2D* renderable, const Maths::Matrix4& transform);
            virtual void BeginSimple();
            virtual void PresentToScreen() override;

            float SubmitTexture(Texture* texture);

            void SetSystemUniforms(Shader* shader) const;

            void CreateGraphicsPipeline();
            void CreateFramebuffers();
            void UpdateDesciptorSet();

            void FlushAndReset();
            void SubmitTriangles();
            void Clear()
            {
                m_Triangles.clear();
                m_CommandQueue2D.clear();
            }

        private:
            void SubmitInternal(const TriangleInfo& triangle);
            void SubmitQueue();

            CommandQueue2D m_CommandQueue2D;
            std::vector<CommandBuffer*> m_SecondaryCommandBuffers;
            std::vector<VertexBuffer*> m_VertexBuffers;

            uint32_t m_BatchDrawCallIndex = 0;
            uint32_t m_IndexCount = 0;

            Render2DLimits m_Limits;

            UniformBuffer* m_UniformBuffer = nullptr;
            IndexBuffer* m_IndexBuffer = nullptr;
            VertexData* m_Buffer = nullptr;
#if !MAP_VERTEX_ARRAY
            std::vector<VertexData*> m_BufferBases;
#endif

            std::vector<Maths::Matrix4> m_TransformationStack;
            const Maths::Matrix4* m_TransformationBack {};

            Texture* m_Textures[MAX_BOUND_TEXTURES];
            uint32_t m_TextureCount;

            uint32_t m_CurrentBufferID = 0;
            Maths::Vector3 m_QuadPositions[4];

            std::vector<TriangleInfo> m_Triangles;

            bool m_Clear = false;
            bool m_RenderToDepthTexture;
            bool m_Empty = false;
            bool m_TriangleIndicies = false;

            uint32_t m_PreviousFrameTextureCount = 0;
        };
    }
}
