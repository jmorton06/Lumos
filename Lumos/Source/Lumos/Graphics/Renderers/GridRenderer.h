#pragma once

#include "IRenderer.h"

namespace Lumos
{
    namespace Graphics
    {
        class Shader;

        class LUMOS_EXPORT GridRenderer : public IRenderer
        {
        public:
            GridRenderer(uint32_t width, uint32_t height);
            ~GridRenderer();

            void Init() override;
            void BeginScene(Scene* scene, Camera* overrideCamera, Maths::Transform* overrideCameraTransform) override;
            void OnResize(uint32_t width, uint32_t height) override;
            void CreateGraphicsPipeline();
            void UpdateUniformBuffer();

            void Begin() override;
            void Submit(const RenderCommand& command) override {};
            void SubmitMesh(Mesh* mesh, Material* material, const Maths::Matrix4& transform, const Maths::Matrix4& textureMatrix) override {};
            void EndScene() override {};
            void End() override;
            void Present() override {};
            void RenderScene() override;
            void PresentToScreen() override { }

            void CreateFramebuffers();

            struct UniformBufferObject
            {
                Lumos::Maths::Matrix4 mvp;
            };

            struct UniformBufferObjectFrag
            {
                Maths::Vector4 cameraPos;
                float scale;
                float res;
                float maxDistance;
                float p1;
            };

            void SetRenderTarget(Texture* texture, bool rebuildFramebuffer) override;
            void OnImGui() override;

        private:
            void SetSystemUniforms(Shader* shader) const;

            Lumos::Graphics::UniformBuffer* m_UniformBuffer;
            Lumos::Graphics::UniformBuffer* m_UniformBufferFrag;

            uint32_t m_CurrentBufferID = 0;
            Mesh* m_Quad;

            float m_GridRes = 1.0f;
            float m_GridSize = 1.0f;
            float m_MaxDistance = 600.0f;
        };
    }
}
