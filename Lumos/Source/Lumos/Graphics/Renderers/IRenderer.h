#pragma once

#include "RenderCommand.h"
#include "Maths/Maths.h"
#include "Maths/Transform.h"

#include "Graphics/RHI/Framebuffer.h"
#include "Graphics/RHI/RenderPass.h"
#include "Graphics/RHI/Pipeline.h"

#define SCENE_DESCRIPTORSET_ID 0
#define MATERIAL_DESCRIPTORSET_ID 1

namespace Lumos
{
    class RenderList;
    class Scene;
    class Camera;

    namespace Graphics
    {
        class DescriptorSet;
        class TextureCube;
        class Texture;
        class Shader;
        class Material;

        typedef std::vector<RenderCommand> CommandQueue;

        class LUMOS_EXPORT IRenderer
        {
        public:
            virtual ~IRenderer();
            virtual void RenderScene() = 0;
            virtual void Init() = 0;
            virtual void Begin() = 0;
            virtual void BeginScene(Scene* scene, Camera* overrideCamera, Maths::Transform* overrideCameraTransform) = 0;
            virtual void Submit(const RenderCommand& command) {};
            virtual void SubmitMesh(Mesh* mesh, Material* material, const Maths::Matrix4& transform, const Maths::Matrix4& textureMatrix) {};
            virtual void EndScene() = 0;
            virtual void End() = 0;
            virtual void Present() = 0;
            virtual void PresentToScreen() = 0;
            virtual void OnResize(uint32_t width, uint32_t height) = 0;
            virtual void OnImGui() {};

            virtual void SetScreenBufferSize(uint32_t width, uint32_t height)
            {
                LUMOS_ASSERT(width != 0 && height != 0, "Width or Height 0!");

                m_ScreenBufferWidth = width;
                m_ScreenBufferHeight = height;
            }

            virtual void SetRenderTarget(Graphics::Texture* texture, bool rebuildFramebuffer = true)
            {
                m_RenderTexture = texture;
            }

            Texture* GetRenderTarget() const
            {
                return m_RenderTexture;
            }

            const SharedRef<Shader>& GetShader() const
            {
                return m_Shader;
            }

            void SetCamera(Camera* camera, Maths::Transform* transform)
            {
                m_Camera = camera;
                m_CameraTransform = transform;
            }

            int GetRenderPriority() const
            {
                return m_RenderPriority;
            }

            void SetRenderPriority(int priority)
            {
                m_RenderPriority = priority;
            }

            bool GetScreenRenderer()
            {
                return m_ScreenRenderer;
            }

            bool GetShouldRender()
            {
                return m_ShouldRender;
            }

        protected:
            Camera* m_Camera = nullptr;
            Maths::Transform* m_CameraTransform = nullptr;

            std::vector<SharedRef<Framebuffer>> m_Framebuffers;
            SharedRef<Shader> m_Shader = nullptr;

            SharedRef<Lumos::Graphics::RenderPass> m_RenderPass;
            SharedRef<Lumos::Graphics::Pipeline> m_Pipeline;
            std::vector<SharedRef<Graphics::DescriptorSet>> m_DescriptorSet;

            std::vector<Graphics::DescriptorSet*> m_CurrentDescriptorSets;

            uint32_t m_ScreenBufferWidth = 0, m_ScreenBufferHeight = 0;
            CommandQueue m_CommandQueue;
            Texture* m_RenderTexture = nullptr;
            Texture* m_DepthTexture = nullptr;

            Maths::Frustum m_Frustum;

            uint8_t* m_VSSystemUniformBuffer = nullptr;
            uint32_t m_VSSystemUniformBufferSize = 0;
            uint8_t* m_PSSystemUniformBuffer = nullptr;
            uint32_t m_PSSystemUniformBufferSize = 0;

            std::vector<uint32_t> m_VSSystemUniformBufferOffsets;
            std::vector<uint32_t> m_PSSystemUniformBufferOffsets;
            Maths::Vector4 m_ClearColour;

            int m_RenderPriority = 0;
            bool m_ScreenRenderer = true;
            bool m_ShouldRender = true;
        };
    }
}
