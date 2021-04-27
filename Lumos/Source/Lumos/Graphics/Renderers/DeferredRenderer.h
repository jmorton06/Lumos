#pragma once
#include "IRenderer.h"

namespace Lumos
{
    class LightSetup;

    namespace Graphics
    {
        class Pipeline;
        class DescriptorSet;
        class GBuffer;
        class Texture2D;
        class TextureDepth;
        class TextureDepthArray;
        class SkyboxRenderer;
        class Shader;
        class ShadowRenderer;
        class Framebuffer;
        class DeferredOffScreenRenderer;

        class LUMOS_EXPORT DeferredRenderer : public IRenderer
        {
        public:
            DeferredRenderer(uint32_t width, uint32_t height);
            ~DeferredRenderer() override;

            void RenderScene() override;

            void Init() override;
            void Begin() override {};
            void Begin(int commandBufferID);
            void BeginScene(Scene* scene, Camera* overrideCamera, Maths::Transform* overrideCameraTransform) override;
            void Submit(const RenderCommand& command) override;
            void SubmitMesh(Mesh* mesh, Material* material, const Maths::Matrix4& transform, const Maths::Matrix4& textureMatrix) override;
            void SubmitLightSetup(Scene* scene);
            void EndScene() override;
            void End() override;
            void Present() override;
            void OnResize(uint32_t width, uint32_t height) override;
            void PresentToScreen() override;

            void CreateDeferredPipeline();
            void CreateLightBuffer();
            void CreateFramebuffers();
            void UpdateScreenDescriptorSet();

            void SetRenderTarget(Texture* texture, bool rebuildFramebuffer) override;

            void OnImGui() override;

        private:
            DeferredOffScreenRenderer* m_OffScreenRenderer;

            void SetSystemUniforms(Shader* shader) const;

            uint8_t* m_PSSystemUniformBuffer;
            uint32_t m_PSSystemUniformBufferSize;

            Maths::Matrix4 m_BiasMatrix;

            UniformBuffer* m_UniformBuffer;
            UniformBuffer* m_LightUniformBuffer;

            CommandBuffer* m_DeferredCommandBuffers;

            Mesh* m_ScreenQuad = nullptr;

            UniqueRef<Texture2D> m_PreintegratedFG;

            int m_CommandBufferIndex = 0;
            int m_RenderMode = 0;

            Texture* m_EnvironmentMap = nullptr;
            Texture* m_IrradianceMap = nullptr;
        };
    }
}
