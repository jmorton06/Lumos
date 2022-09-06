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
            void SubmitMesh(Mesh* mesh, Material* material, const glm::mat4& transform, const glm::mat4& textureMatrix) override {};
            void EndScene() override {};
            void End() override;
            void Present() override {};
            void RenderScene() override;
            void PresentToScreen() override { }

            void CreateFramebuffers();

            struct UBOFrag
            {
                glm::vec4 cameraPos;
                glm::vec4 cameraForward;
                float Near;
                float Far;
                float maxDistance;
                float p1;
            };

            void SetRenderTarget(Texture* texture, bool rebuildFramebuffer) override;
            void OnImGui() override;

        private:
            uint32_t m_CurrentBufferID = 0;
            Mesh* m_Quad;

            float m_GridRes     = 1.0f;
            float m_GridSize    = 1.0f;
            float m_MaxDistance = 600.0f;
        };
    }
}
