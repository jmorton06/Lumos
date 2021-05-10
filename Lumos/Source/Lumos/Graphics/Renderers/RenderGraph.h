#pragma once
#include "Scene/Scene.h"

namespace Lumos
{
    enum class RenderPriority
    {
        Geometry = 0,
        Lighting = 1,
        Geometry2D = 2,
        PostProcess = 3,
        Debug = 4,
        ImGui = 5,
        Screen = 6,
        Total = 7
    };

    namespace Maths
    {
        class Transform;
    }

    namespace Graphics
    {
        class IRenderer;
        class Texture;
        class GBuffer;
        class TextureDepthArray;
        class ShadowRenderer;
        class SkyboxRenderer;

        class RenderGraph
        {
        public:
            RenderGraph(uint32_t width, uint32_t height);
            ~RenderGraph();

            void AddRenderer(Graphics::IRenderer* renderer);
            void AddRenderer(Graphics::IRenderer* renderer, int renderPriority);

            void SortRenderers();
            void EnableDebugRenderer(bool enable);

            void Reset();
            void OnResize(uint32_t width, uint32_t height);
            void BeginScene(Scene* scene);
            void OnNewScene(Scene* scene);

            void OnRender();
            void OnUpdate(const TimeStep& timeStep, Scene* scene);
            void OnEvent(Event& e);
            void OnImGui();

            bool GetReflectSkyBox() const { return m_ReflectSkyBox; };
            bool GetUseShadowMap() const { return m_UseShadowMap; };
            uint32_t GetNumShadowMaps() const { return m_NumShadowMaps; };
            TextureDepthArray* GetShadowTexture() const { return m_ShadowTexture; };
            GBuffer* GetGBuffer() const { return m_GBuffer; }

            void SetReflectSkyBox(bool reflect) { m_ReflectSkyBox = reflect; }
            void SetUseShadowMap(bool shadow) { m_UseShadowMap = shadow; }
            void SetNumShadowMaps(uint32_t num) { m_NumShadowMaps = num; }
            void SetTextureDepthArray(TextureDepthArray* texture) { m_ShadowTexture = texture; }

            ShadowRenderer* GetShadowRenderer() const { return m_ShadowRenderer; };
            void SetShadowRenderer(ShadowRenderer* renderer) { m_ShadowRenderer = renderer; }

            void SetScreenBufferSize(uint32_t width, uint32_t height)
            {
                if(width == 0)
                    width = 1;
                if(height == 0)
                    height = 1;
                m_ScreenBufferWidth = width;
                m_ScreenBufferHeight = height;
            }

            void SetRenderTarget(Graphics::Texture* texture, bool onlyIfTargetsScreen = false, bool rebuildFramebuffer = true);

            void SetOverrideCamera(Camera* camera, Maths::Transform* overrideCameraTransform)
            {
                m_OverrideCamera = camera;
                m_OverrideCameraTransform = overrideCameraTransform;
            }

            bool OnwindowResizeEvent(WindowResizeEvent& e);
            uint32_t GetCount() const { return (uint32_t)m_Renderers.size(); }

        private:
            std::vector<Graphics::IRenderer*> m_Renderers;

            bool m_ReflectSkyBox = false;
            bool m_UseShadowMap = false;
            uint32_t m_NumShadowMaps = 4;
            TextureDepthArray* m_ShadowTexture = nullptr;
            Texture* m_ScreenTexture = nullptr;

            GBuffer* m_GBuffer = nullptr;

            ShadowRenderer* m_ShadowRenderer = nullptr;

            Camera* m_OverrideCamera = nullptr;
            Maths::Transform* m_OverrideCameraTransform = nullptr;

            uint32_t m_ScreenBufferWidth {}, m_ScreenBufferHeight {};
        };
    }
}
