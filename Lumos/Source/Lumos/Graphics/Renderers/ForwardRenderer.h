#pragma once

#include "IRenderer.h"
#include "Maths/Frustum.h"

namespace Lumos
{
    class LightSetup;

    namespace Graphics
    {
        class DescriptorSet;
        class TextureDepth;

        class LUMOS_EXPORT ForwardRenderer : public IRenderer
        {
        public:
            ForwardRenderer(uint32_t width, uint32_t height, bool depthTest = true);
            ~ForwardRenderer() override;
            void RenderScene() override;

            void Init() override;
            void Begin() override;
            void BeginScene(Scene* scene, Camera* overrideCamera, Maths::Transform* overrideCameraTransform) override;

            void BeginScene(const Maths::Matrix4& proj, const Maths::Matrix4& view);
            void Submit(const RenderCommand& command) override;
            void SubmitMesh(Mesh* mesh, Material* material, const Maths::Matrix4& transform, const Maths::Matrix4& textureMatrix) override;
            void EndScene() override;
            void End() override;
            void Present() override;
            void OnResize(uint32_t width, uint32_t height) override;
            void PresentToScreen() override { }
            void SetRenderTarget(Texture* texture, bool rebuildFramebuffer) override;

            void CreateGraphicsPipeline();
            void CreateFramebuffers();

            struct UniformBufferObject
            {
                Lumos::Maths::Matrix4 proj;
                Lumos::Maths::Matrix4 view;
            };

            struct UniformBufferModel
            {
                Lumos::Maths::Matrix4* model;
            };

            void SetSystemUniforms(Shader* shader) const;

        private:
            Texture2D* m_DefaultTexture;

            UniformBuffer* m_UniformBuffer;
            UniformBuffer* m_ModelUniformBuffer;

            std::vector<Lumos::Graphics::CommandBuffer*> m_CommandBuffers;

            size_t m_DynamicAlignment;
            UniformBufferModel m_UBODataDynamic;

            uint32_t m_CurrentBufferID = 0;
            bool m_DepthTest = false;
        };
    }
}
