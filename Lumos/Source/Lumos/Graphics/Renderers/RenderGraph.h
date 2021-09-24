#pragma once
#include "Scene/Scene.h"
#include "Graphics/Renderers/IRenderer.h"
#include "Graphics/Renderable2D.h"

#define MAX_BOUND_TEXTURES 16

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
        class SkyboxRenderer;
        class CommandBuffer;

        struct LineVertexData
        {
            Maths::Vector3 vertex;
            Maths::Vector4 colour;

            bool operator==(const LineVertexData& other) const
            {
                return vertex == other.vertex && colour == other.colour;
            }
        };

        struct PointVertexData
        {
            Maths::Vector3 vertex;
            Maths::Vector4 colour;
            Maths::Vector2 size;
            Maths::Vector2 uv;

            bool operator==(const PointVertexData& other) const
            {
                return vertex == other.vertex && colour == other.colour && size == other.size && uv == other.uv;
            }
        };

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
            GBuffer* GetGBuffer() const { return m_GBuffer; }

            void SetReflectSkyBox(bool reflect) { m_ReflectSkyBox = reflect; }
            void SetUseShadowMap(bool shadow) { m_UseShadowMap = shadow; }

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

            void ForwardPass();
            void ShadowPass();
            void SkyboxPass();
            void Render2DPass();
            void DebugPass();
            float SubmitTexture(Texture* texture);
            void UpdateCascades(Scene* scene, Light* light);

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

            struct ShadowData
            {
                uint32_t m_Layer = 0;
                float m_CascadeSplitLambda;
                float m_SceneRadiusMultiplier;

                float m_LightSize;
                float m_MaxShadowDistance;
                float m_ShadowFade;
                float m_CascadeTransitionFade;
                float m_InitialBias;
                CommandQueue m_CascadeCommandQueue[SHADOWMAP_MAX];

                TextureDepthArray* m_ShadowTex;
                uint32_t m_ShadowMapNum;
                uint32_t m_ShadowMapSize;
                bool m_ShadowMapsInvalidated;
                Maths::Matrix4 m_ShadowProjView[SHADOWMAP_MAX];
                Maths::Vector4 m_SplitDepth[SHADOWMAP_MAX];
                Maths::Matrix4 m_LightMatrix;
                std::vector<SharedPtr<Graphics::DescriptorSet>> m_DescriptorSet;

                std::vector<Graphics::DescriptorSet*> m_CurrentDescriptorSets;
                SharedPtr<Shader> m_Shader = nullptr;
                Maths::Frustum m_CascadeFrustums[SHADOWMAP_MAX];
            };

            struct ForwardData
            {
                Texture2D* m_DefaultTexture;
                Material* m_DefaultMaterial;

                UniquePtr<Texture2D> m_PreintegratedFG;
                std::vector<Lumos::Graphics::CommandBuffer*> m_CommandBuffers;

                Maths::Matrix4 m_BiasMatrix;
                Texture* m_EnvironmentMap = nullptr;
                Texture* m_IrradianceMap = nullptr;

                CommandQueue m_CommandQueue;

                std::vector<SharedPtr<Graphics::DescriptorSet>> m_DescriptorSet;

                std::vector<Graphics::DescriptorSet*> m_CurrentDescriptorSets;
                SharedPtr<Shader> m_Shader = nullptr;
                Texture* m_RenderTexture = nullptr;
                Texture* m_DepthTexture = nullptr;

                Maths::Frustum m_Frustum;
                Maths::Vector4 m_ClearColour;

                uint32_t m_RenderMode = 0;
                uint32_t m_CurrentBufferID = 0;
                bool m_DepthTest = false;
            };

            struct Renderer2DData
            {
                CommandQueue2D m_CommandQueue2D;
                std::vector<std::vector<VertexBuffer*>> m_VertexBuffers;

                uint32_t m_BatchDrawCallIndex = 0;
                uint32_t m_IndexCount = 0;

                Render2DLimits m_Limits;

                IndexBuffer* m_IndexBuffer = nullptr;
                VertexData* m_Buffer = nullptr;

                std::vector<Maths::Matrix4> m_TransformationStack;
                const Maths::Matrix4* m_TransformationBack {};

                Texture* m_Textures[MAX_BOUND_TEXTURES];
                uint32_t m_TextureCount;

                uint32_t m_CurrentBufferID = 0;
                Maths::Vector3 m_QuadPositions[4];

                bool m_Clear = false;
                bool m_RenderToDepthTexture;
                bool m_Empty = false;
                bool m_TriangleIndicies = false;

                uint32_t m_PreviousFrameTextureCount = 0;
                SharedPtr<Shader> m_Shader = nullptr;

                std::vector<SharedPtr<Graphics::DescriptorSet>> m_DescriptorSet;
                std::vector<Graphics::DescriptorSet*> m_CurrentDescriptorSets;
            };

            struct DebugDrawData
            {
                std::vector<Graphics::VertexBuffer*> m_LineVertexBuffers;
                Graphics::IndexBuffer* m_LineIndexBuffer;

                Graphics::IndexBuffer* m_PointIndexBuffer = nullptr;
                std::vector<Graphics::VertexBuffer*> m_PointVertexBuffers;

                std::vector<SharedPtr<Graphics::DescriptorSet>> m_LineDescriptorSet;
                std::vector<SharedPtr<Graphics::DescriptorSet>> m_PointDescriptorSet;

                LineVertexData* m_LineBuffer = nullptr;
                PointVertexData* m_PointBuffer = nullptr;

                uint32_t LineIndexCount = 0;
                uint32_t PointIndexCount = 0;
                uint32_t m_LineBatchDrawCallIndex = 0;
                uint32_t m_PointBatchDrawCallIndex = 0;

                Renderer2DData m_Renderer2DData;

                SharedPtr<Shader> m_LineShader = nullptr;
                SharedPtr<Shader> m_PointShader = nullptr;
            };

            ForwardData& GetForwardData() { return m_ForwardData; }
            ShadowData& GetShadowData() { return m_ShadowData; }

        private:
            std::vector<Graphics::IRenderer*> m_Renderers;

            bool m_ReflectSkyBox = false;
            bool m_UseShadowMap = false;

            Texture* m_ScreenTexture = nullptr;

            GBuffer* m_GBuffer = nullptr;

            Camera* m_Camera = nullptr;
            Maths::Transform* m_CameraTransform = nullptr;

            Camera* m_OverrideCamera = nullptr;
            Maths::Transform* m_OverrideCameraTransform = nullptr;

            uint32_t m_ScreenBufferWidth {}, m_ScreenBufferHeight {};

            ShadowData m_ShadowData;
            ForwardData m_ForwardData;
            Renderer2DData m_Renderer2DData;
            DebugDrawData m_DebugDrawData;

            Mesh* m_SkyboxMesh;
            Texture* m_CubeMap;
            SharedPtr<Graphics::Shader> m_SkyboxShader;
            SharedPtr<Graphics::DescriptorSet> m_SkyboxDescriptorSet;
        };
    }
}
