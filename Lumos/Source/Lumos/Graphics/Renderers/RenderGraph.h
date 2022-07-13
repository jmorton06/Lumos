#pragma once
#include "Scene/Scene.h"
#include "Graphics/Renderers/IRenderer.h"
#include "Graphics/Renderable2D.h"

#define MAX_BOUND_TEXTURES 16

namespace Lumos
{
    namespace Maths
    {
        class Transform;
    }

    namespace Graphics
    {
        class IRenderer;
        class Texture;
        class TextureDepth;
        class GBuffer;
        class TextureDepthArray;
        class SkyboxRenderer;
        class CommandBuffer;

        struct LineVertexData
        {
            glm::vec3 vertex;
            glm::vec4 colour;

            bool operator==(const LineVertexData& other) const
            {
                return vertex == other.vertex && colour == other.colour;
            }
        };

        struct PointVertexData
        {
            glm::vec3 vertex;
            glm::vec4 colour;
            glm::vec2 size;
            glm::vec2 uv;

            bool operator==(const PointVertexData& other) const
            {
                return vertex == other.vertex && colour == other.colour && size == other.size && uv == other.uv;
            }
        };

        struct RenderGraphSettings
        {
            bool DebugPass       = true;
            bool GeomPass        = true;
            bool PostProcessPass = false;
            bool ShadowPass      = true;
            bool SkyboxPass      = true;
        };

        struct RenderGraphStats
        {
            uint32_t UpdatesPerSecond;
            uint32_t FramesPerSecond;
            uint32_t NumRenderedObjects = 0;
            uint32_t NumShadowObjects   = 0;
            uint32_t NumDrawCalls       = 0;
        };

        class RenderGraph
        {
        public:
            RenderGraph(uint32_t width, uint32_t height);
            ~RenderGraph();

            void EnableDebugRenderer(bool enable);

            void OnResize(uint32_t width, uint32_t height);
            void BeginScene(Scene* scene);
            void OnNewScene(Scene* scene);

            void OnRender();
            void OnUpdate(const TimeStep& timeStep, Scene* scene);
            void OnEvent(Event& e);
            void OnImGui();

            void SetRenderTarget(Graphics::Texture* texture, bool onlyIfTargetsScreen = false, bool rebuildFramebuffer = true);

            void SetOverrideCamera(Camera* camera, Maths::Transform* overrideCameraTransform)
            {
                m_OverrideCamera          = camera;
                m_OverrideCameraTransform = overrideCameraTransform;
            }

            bool OnwindowResizeEvent(WindowResizeEvent& e);

            void GenerateBRDFLUTPass();
            void DepthPrePass();
            void SSAOPass();
            void ForwardPass();
            void ShadowPass();
            void SkyboxPass();
            void Render2DPass();
            void Render2DFlush();
            void DebugPass();
            void FinalPass();

            // Post Process
            void ToneMappingPass();
            void BloomPass();
            void FXAAPass();
            void DebandingPass();
            void ChromaticAberationPass();
            void EyeAdaptationPass();
            void FilmicGrainPass();
            void OutlinePass();

            float SubmitTexture(Texture* texture);
            void UpdateCascades(Scene* scene, Light* light);

            struct LUMOS_EXPORT RenderCommand2D
            {
                Renderable2D* renderable = nullptr;
                glm::mat4 transform;
            };

            typedef std::vector<RenderCommand2D> CommandQueue2D;

            struct Render2DLimits
            {
                uint32_t MaxQuads          = 10000;
                uint32_t QuadsSize         = RENDERER2D_VERTEX_SIZE * 4;
                uint32_t BufferSize        = 10000 * RENDERER2D_VERTEX_SIZE * 4;
                uint32_t IndiciesSize      = 10000 * 6;
                uint32_t MaxTextures       = 16;
                uint32_t MaxBatchDrawCalls = 100;

                void SetMaxQuads(uint32_t quads)
                {
                    MaxQuads     = quads;
                    BufferSize   = MaxQuads * RENDERER2D_VERTEX_SIZE * 4;
                    IndiciesSize = MaxQuads * 6;
                }
            };

            struct ShadowData
            {
                uint32_t m_Layer = 0;
                float m_CascadeSplitLambda;
                float m_LightSize;
                float m_MaxShadowDistance;
                float m_ShadowFade;
                float m_CascadeTransitionFade;
                float m_InitialBias;
                float CascadeFarPlaneOffset = 50.0f, CascadeNearPlaneOffset = -50.0f;
                CommandQueue m_CascadeCommandQueue[SHADOWMAP_MAX];

                TextureDepthArray* m_ShadowTex;
                uint32_t m_ShadowMapNum;
                uint32_t m_ShadowMapSize;
                bool m_ShadowMapsInvalidated;
                glm::mat4 m_ShadowProjView[SHADOWMAP_MAX];
                glm::vec4 m_SplitDepth[SHADOWMAP_MAX];
                glm::mat4 m_LightMatrix;
                std::vector<SharedPtr<Graphics::DescriptorSet>> m_DescriptorSet;

                std::vector<Graphics::DescriptorSet*> m_CurrentDescriptorSets;
                SharedPtr<Shader> m_Shader = nullptr;
                Maths::Frustum m_CascadeFrustums[SHADOWMAP_MAX];
            };

            struct ForwardData
            {
                Texture2D* m_DefaultTexture;
                Material* m_DefaultMaterial;

                UniquePtr<Texture2D> m_BRDFLUT;
                std::vector<Lumos::Graphics::CommandBuffer*> m_CommandBuffers;

                glm::mat4 m_BiasMatrix;
                Texture* m_EnvironmentMap = nullptr;
                Texture* m_IrradianceMap  = nullptr;

                CommandQueue m_CommandQueue;

                std::vector<SharedPtr<Graphics::DescriptorSet>> m_DescriptorSet;
                std::vector<Graphics::DescriptorSet*> m_CurrentDescriptorSets;

                SharedPtr<Shader> m_Shader   = nullptr;
                Texture* m_RenderTexture     = nullptr;
                TextureDepth* m_DepthTexture = nullptr;

                Maths::Frustum m_Frustum;

                uint32_t m_RenderMode      = 0;
                uint32_t m_CurrentBufferID = 0;
                bool m_DepthTest           = false;
                size_t m_DynamicAlignment;
                glm::mat4* m_TransformData = nullptr;
            };

            struct Renderer2DData
            {
                CommandQueue2D m_CommandQueue2D;
                std::vector<std::vector<VertexBuffer*>> m_VertexBuffers;

                uint32_t m_BatchDrawCallIndex = 0;
                uint32_t m_IndexCount         = 0;

                Render2DLimits m_Limits;

                IndexBuffer* m_IndexBuffer = nullptr;
                VertexData* m_Buffer       = nullptr;

                std::vector<glm::mat4> m_TransformationStack;
                const glm::mat4* m_TransformationBack {};

                Texture* m_Textures[MAX_BOUND_TEXTURES];
                uint32_t m_TextureCount;

                uint32_t m_CurrentBufferID = 0;
                glm::vec3 m_QuadPositions[4];

                bool m_RenderToDepthTexture;
                bool m_TriangleIndicies = false;

                std::vector<uint32_t> m_PreviousFrameTextureCount;
                SharedPtr<Shader> m_Shader     = nullptr;
                SharedPtr<Pipeline> m_Pipeline = nullptr;

                std::vector<std::vector<SharedPtr<Graphics::DescriptorSet>>> m_DescriptorSet;
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

                LineVertexData* m_LineBuffer   = nullptr;
                PointVertexData* m_PointBuffer = nullptr;

                uint32_t LineIndexCount            = 0;
                uint32_t PointIndexCount           = 0;
                uint32_t m_LineBatchDrawCallIndex  = 0;
                uint32_t m_PointBatchDrawCallIndex = 0;

                Renderer2DData m_Renderer2DData;

                SharedPtr<Shader> m_LineShader  = nullptr;
                SharedPtr<Shader> m_PointShader = nullptr;
            };

            ForwardData& GetForwardData() { return m_ForwardData; }
            ShadowData& GetShadowData() { return m_ShadowData; }
            RenderGraphSettings& GetSettings() { return m_Settings; }
            RenderGraphStats& GetRenderGraphStats() { return m_Stats; }

            SharedPtr<TextureCube> CreateCubeFromHDRI(const std::string& filePath);

        private:
            Texture2D* m_MainTexture      = nullptr;
            Texture2D* m_LastRenderTarget = nullptr;

            Texture2D* m_PostProcessTexture1 = nullptr;
            Texture2D* m_PostProcessTexture2 = nullptr;

            Camera* m_Camera                    = nullptr;
            Maths::Transform* m_CameraTransform = nullptr;

            Camera* m_OverrideCamera                    = nullptr;
            Maths::Transform* m_OverrideCameraTransform = nullptr;

            ShadowData m_ShadowData;
            ForwardData m_ForwardData;
            Renderer2DData m_Renderer2DData;
            DebugDrawData m_DebugDrawData;
            glm::vec4 m_ClearColour;

            int m_ToneMapIndex     = 4;
            float m_Exposure       = 1.0f;
            float m_BloomIntensity = 1.0f;
            Scene* m_CurrentScene  = nullptr;
            bool m_GenerateBRDFLUT = false;
            bool m_SupportCompute  = false;

            Mesh* m_ScreenQuad;
            Texture* m_CubeMap;
            Texture* m_DefaultTextureCube;
            SharedPtr<Graphics::Shader> m_SkyboxShader;
            SharedPtr<Graphics::DescriptorSet> m_SkyboxDescriptorSet;

            SharedPtr<Graphics::Shader> m_FinalPassShader;
            SharedPtr<Graphics::DescriptorSet> m_FinalPassDescriptorSet;

            Texture2D* m_BloomTexture               = nullptr;
            Texture2D* m_BloomTexture1              = nullptr;
            Texture2D* m_BloomTexture2              = nullptr;
            Texture2D* m_BloomTextureLastRenderered = nullptr;

            SharedPtr<Graphics::Shader> m_BloomPassShader;
            SharedPtr<Graphics::DescriptorSet> m_BloomPassDescriptorSet;
            SharedPtr<Graphics::DescriptorSet> m_BloomPassDescriptorSet1;
            SharedPtr<Graphics::DescriptorSet> m_BloomPassDescriptorSet15;
            SharedPtr<Graphics::DescriptorSet> m_BloomPassDescriptorSet2;
            SharedPtr<Graphics::DescriptorSet> m_BloomPassDescriptorSet3;
            SharedPtr<Graphics::DescriptorSet> m_BloomPassDescriptorSet4;

            SharedPtr<Graphics::DescriptorSet> m_FXAAPassDescriptorSet;
            SharedPtr<Graphics::Shader> m_FXAAShader;

            SharedPtr<Graphics::DescriptorSet> m_DebandingPassDescriptorSet;
            SharedPtr<Graphics::Shader> m_DebandingShader;

            SharedPtr<Graphics::DescriptorSet> m_ChromaticAberationPassDescriptorSet;
            SharedPtr<Graphics::Shader> m_ChromaticAberationShader;

            SharedPtr<Graphics::DescriptorSet> m_DepthPrePassDescriptorSet;
            SharedPtr<Pipeline> m_DepthPrePassPipeline = nullptr;
            SharedPtr<Graphics::Shader> m_DepthPrePassShader;

            TextureDepth* m_SSAOTexture = nullptr;
            SharedPtr<Graphics::Shader> m_SSAOShader;
            SharedPtr<Graphics::DescriptorSet> m_SSAOPassDescriptorSet;

            SharedPtr<Graphics::Shader> m_ToneMappingPassShader;
            SharedPtr<Graphics::DescriptorSet> m_ToneMappingPassDescriptorSet;

            SharedPtr<Graphics::DescriptorSet> m_FilmicGrainPassDescriptorSet;
            SharedPtr<Graphics::Shader> m_FilmicGrainShader;

            //SharedPtr<Graphics::DescriptorSet> m_OutlinePassDescriptorSet;
            SharedPtr<Graphics::Shader> m_OutlineShader;

            RenderGraphSettings m_Settings;
            RenderGraphStats m_Stats;
        };
    }
}
