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
        class Material;

        class LUMOS_EXPORT ForwardRenderer : public IRenderer
        {
        public:
            ForwardRenderer(uint32_t width, uint32_t height, bool depthTest = true);
            ~ForwardRenderer() override;
            void RenderScene() override;

            void Init() override;
            void Begin() override;
            void BeginScene(Scene* scene, Camera* overrideCamera, Maths::Transform* overrideCameraTransform) override;
            void Submit(const RenderCommand& command) override;
            void SubmitMesh(Mesh* mesh, Material* material, const Maths::Matrix4& transform, const Maths::Matrix4& textureMatrix) override;
            void SubmitLightSetup(Scene* scene);
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
                Lumos::Maths::Matrix4 projview;
            };

            struct UniformBufferModel
            {
                Lumos::Maths::Matrix4* model;
            };

            void SetSystemUniforms(Shader* shader) const;
            void UpdateScreenDescriptorSet();
            
        private:
            Texture2D* m_DefaultTexture;
            Material* m_DefaultMaterial;

            UniformBuffer* m_UniformBuffer;
            UniformBuffer* m_LightUniformBuffer;
            UniqueRef<Texture2D> m_PreintegratedFG;

            std::vector<Lumos::Graphics::CommandBuffer*> m_CommandBuffers;

            int m_RenderMode = 0;
            
            Maths::Matrix4 m_BiasMatrix;

            Texture* m_EnvironmentMap = nullptr;
            Texture* m_IrradianceMap = nullptr;
            
            uint8_t* m_PSSceneUniformBuffer = nullptr;
            std::vector<uint32_t> m_PSSceneUniformBufferOffsets;

            uint32_t m_PSSceneUniformBufferSize = 0;
            uint32_t m_CurrentBufferID = 0;
            bool m_DepthTest = false;
        };
    }
}
