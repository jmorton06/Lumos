#pragma once
#include "IRenderer.h"
#include "Maths/Frustum.h"

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
        class Material;

        class LUMOS_EXPORT DeferredOffScreenRenderer : public IRenderer
        {
        public:
            DeferredOffScreenRenderer(uint32_t width, uint32_t height);
            ~DeferredOffScreenRenderer() override;

            void RenderScene() override;

            void Init() override;
            void Begin() override;
            void BeginScene(Scene* scene, Camera* overrideCamera, Maths::Transform* overrideCameraTransforms) override;
            void Submit(const RenderCommand& command) override;
            void SubmitMesh(Mesh* mesh, Material* material, const Maths::Matrix4& transform, const Maths::Matrix4& textureMatrix) override;
            void EndScene() override;
            void End() override;
            void Present() override;
            void OnResize(uint32_t width, uint32_t height) override;
            void PresentToScreen() override { }

            void CreatePipeline();
            void CreateBuffer();
            void CreateFramebuffer();

            void OnImGui() override;

            bool HadRendered() const { return m_HasRendered; }

        private:
            Material* m_DefaultMaterial;

            SharedPtr<Shader> m_AnimatedShader = nullptr;
            SharedPtr<Lumos::Graphics::Pipeline> m_AnimatedPipeline;

            bool m_HasRendered = false;
        };
    }
}
