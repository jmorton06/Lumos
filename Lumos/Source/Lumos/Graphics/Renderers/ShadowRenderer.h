#pragma once

#include "Maths/Maths.h"
#include "IRenderer.h"

#include <entt/entity/fwd.hpp>
#define SHADOWMAP_MAX 16

namespace Lumos
{
    class RenderList;
    class Scene;
    class Camera;
    class Mesh;

    namespace Graphics
    {
        struct Light;
        struct RenderCommand;
        class Shader;
        class TextureDepthArray;
        class Framebuffer;
        class Pipeline;
        class DescriptorSet;
        class UniformBuffer;
        class CommandBuffer;
        class RenderPass;

        typedef std::vector<RenderCommand> CommandQueue;

        class LUMOS_EXPORT ShadowRenderer : public IRenderer
        {
        public:
            ShadowRenderer(TextureDepthArray* texture = nullptr, uint32_t shadowMapSize = 2048, uint32_t numMaps = 4);
            ~ShadowRenderer();

            ShadowRenderer(ShadowRenderer const&) = delete;
            ShadowRenderer& operator=(ShadowRenderer const&) = delete;

            void Init() override;
            void BeginScene(Scene* scene, Camera* overrideCamera, Maths::Transform* overrideCameraTransform) override;
            void OnResize(uint32_t width, uint32_t height) override;

            void SetShadowMapNum(uint32_t num);
            void SetShadowMapSize(uint32_t size);

            void Begin() override;
            void Submit(const RenderCommand& command) override;
            void Submit(const RenderCommand& command, uint32_t cascadeIndex);
            void SubmitMesh(Mesh* mesh, Material* material, const Maths::Matrix4& transform, const Maths::Matrix4& textureMatrix, uint32_t cascadeIndex);

            void SubmitMesh(Mesh* mesh, Material* material, const Maths::Matrix4& transform, const Maths::Matrix4& textureMatrix) override;
            void EndScene() override;
            void End() override;
            void Present() override;
            void RenderScene() override;
            void PresentToScreen() override { }

            Maths::Vector4* GetSplitDepths()
            {
                return m_SplitDepth;
            }
            Maths::Matrix4* GetShadowProjView()
            {
                return m_ShadowProjView;
            }

            inline uint32_t GetShadowMapSize() const
            {
                return m_ShadowMapSize;
            }
            inline uint32_t GetShadowMapNum() const
            {
                return m_ShadowMapNum;
            }
            inline void SetShadowInvalid()
            {
                m_ShadowMapsInvalidated = true;
            }

            inline TextureDepthArray* GetTexture() const
            {
                return m_ShadowTex;
            }

            uint8_t* m_VSSystemUniformBuffer {};
            uint32_t m_VSSystemUniformBufferSize {};

            std::vector<uint32_t> m_VSSystemUniformBufferOffsets;

            struct UniformBufferObject
            {
                Lumos::Maths::Matrix4 projView[SHADOWMAP_MAX];
            };

            void CreateGraphicsPipeline();
            void CreateFramebuffers();
            void CreateUniformBuffer();
            void UpdateCascades(Scene* scene, Camera* overrideCamera, Maths::Transform* overrideCameraTransform, Light* light);

            const Lumos::Maths::Matrix4& GetLightView() const { return m_LightMatrix; }

            void OnImGui() override;

            float GetLightSize() { return m_LightSize; }
            float GetMaxShadowDistance() { return m_MaxShadowDistance; }
            float GetShadowFade() { return m_ShadowFade; }
            float GetCascadeTransitionFade() { return m_CascadeTransitionFade; }
            float GetInitialBias() { return m_InitialBias; }

        protected:
            void SetSystemUniforms(Shader* shader);

            TextureDepthArray* m_ShadowTex;
            uint32_t m_ShadowMapNum;
            uint32_t m_ShadowMapSize;
            bool m_ShadowMapsInvalidated;
            SharedRef<Framebuffer> m_ShadowFramebuffer[SHADOWMAP_MAX] {};
            Maths::Matrix4 m_ShadowProjView[SHADOWMAP_MAX];
            Maths::Vector4 m_SplitDepth[SHADOWMAP_MAX];
            Maths::Matrix4 m_LightMatrix;

            CommandQueue m_CascadeCommandQueue[SHADOWMAP_MAX];

            Lumos::Graphics::UniformBuffer* m_UniformBuffer;

            uint32_t m_Layer = 0;
            float m_CascadeSplitLambda;
            float m_SceneRadiusMultiplier;

            float m_LightSize;
            float m_MaxShadowDistance;
            float m_ShadowFade;
            float m_CascadeTransitionFade;
            float m_InitialBias;
        };
    }
}
