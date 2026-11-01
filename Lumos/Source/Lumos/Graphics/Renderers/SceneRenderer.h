#pragma once
#include "Graphics/Renderers/IRenderer.h"
#include "Graphics/Renderable2D.h"
#include "Graphics/Renderers/RenderPassInfo.h"

#define MAX_BOUND_TEXTURES 16

namespace Lumos
{
    class Scene;
    class TimeStep;
    class WindowResizeEvent;
    class Event;
    struct SceneRenderSettings;
    struct UI_Widget;
    struct UI_State;

    namespace Maths
    {
        class Transform;
        class Frustum;
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
        class Model;
        struct Light;
        struct GPULight;
        class VertexBuffer;
        class IndexBuffer;
        class StorageBuffer;
        class ProbeGI;

        struct LineVertexData
        {
            Vec3 vertex;
            Vec4 colour;
            Vec4 edge; // .x = cross-line coord (-1..1) for SDF anti-aliasing of thick lines; 0 for hairlines. Must stay 16B to match vertex-layout reflection.

            bool operator==(const LineVertexData& other) const
            {
                return vertex == other.vertex && colour == other.colour && edge == other.edge;
            }
        };

        struct PointVertexData
        {
            Vec3 vertex;
            Vec4 colour;
            Vec2 size;
            Vec2 uv;

            bool operator==(const PointVertexData& other) const
            {
                return vertex == other.vertex && colour == other.colour && size == other.size && uv == other.uv;
            }
        };

        struct InstanceBatchInfo
        {
            const char* meshName = nullptr;
            uint32_t instanceCount = 0;
        };

        struct SceneRendererStats
        {
            uint32_t UpdatesPerSecond;
            uint32_t FramesPerSecond;
            uint32_t NumRenderedObjects  = 0;
            uint32_t NumShadowObjects    = 0;
            uint32_t NumDrawCalls        = 0;
            uint32_t NumInstanceBatches  = 0;
            uint32_t NumInstancedObjects = 0;
            uint32_t NumOcclusionCulled  = 0;
            uint32_t NumSmallCulled      = 0; // under RenderSettings::MinScreenSize
            uint32_t NumLODMeshes        = 0; // meshes drawn below LOD 0
            static constexpr uint32_t MaxTrackedBatches = 16;
            InstanceBatchInfo InstanceBatches[MaxTrackedBatches];
        };

        class SceneRenderer
        {
        public:
            SceneRenderer(uint32_t width, uint32_t height);
            ~SceneRenderer();

            void EnableDebugRenderer(bool enable);

            void SetClearColour(const Vec4& colour) { m_ClearColour = colour; }
            const Vec4& GetClearColour() const { return m_ClearColour; }
            // Clear to alpha 0 and carry alpha to the swapchain (needs a transparent swapchain + window)
            void SetTransparentBackground(bool transparent) { m_TransparentBackground = transparent; }
            bool GetTransparentBackground() const { return m_TransparentBackground; }

            void OnResize(uint32_t width, uint32_t height);
            // Multiplies the quality render scale for this renderer only (editor views); callers resize after.
            void SetViewScale(float scale) { m_ViewScale = scale; }
            float GetViewScale() const { return m_ViewScale; }
            void BeginScene(Scene* scene);
            void OnNewScene(Scene* scene);

            void OnRender();
            void OnUpdate(const TimeStep& timeStep, Scene* scene);
            void OnEvent(Event& e);
            void OnImGui();

            void SetRenderTarget(Graphics::Texture* texture, bool onlyIfTargetsScreen = false, bool rebuildFramebuffer = true);

            // Whatever the frame is drawn from (the editor's camera when it has taken over).
            Maths::Transform* GetViewTransform() const { return m_OverrideCameraTransform ? m_OverrideCameraTransform : m_CameraTransform; }
            void SetOverrideCamera(Camera* camera, Maths::Transform* overrideCameraTransform)
            {
                m_OverrideCamera          = camera;
                m_OverrideCameraTransform = overrideCameraTransform;
            }

            bool OnWindowResizeEvent(WindowResizeEvent& e);

            void GenerateBRDFLUTPass();
            void DepthPrePass();
            void SSAOPass();
            void OcclusionGridPass();
            void SSAOBlurPass();
            void ForwardPass();
            void LightCullingPass();
            void ShadowPass();
            void SpotShadowPass(bool* layerDrawn);
            void SkyboxPass();
            void CosmoPass();
            void CosmoBake();
            void Renderer2DBeginBatch();
            void Render2DPass();
            void Render2DFlush();
            void Init2DLitRenderData();
            void Lit2DBeginBatch();
            void Render2DLitPass();
            void Lit2DFlush();
            float SubmitLitTexture(Texture* texture);
            void ParticleBeginBatch();
            void ParticlePass();
            void ParticleFlush();
            void PointCloudPass();
            void LineCloudPass();
            void PlanetPass();
            void DebugPass();
            void DebugLineFlush(Graphics::Pipeline* pipeline);
            void DebugThickLineFlush(Graphics::Pipeline* pipeline);
            void DebugPointFlush(Graphics::Pipeline* pipeline);
            void FinalPass();
            void TextPass();
            void WorldTextPass();

            void Begin2DPass();
            void BeginTextPass();
            void draw_ui(UI_Widget* widget);                       // widget + subtree (tree order = z-order)
            void draw_ui_widget(UI_Widget* widget, int part = 0);  // single widget, no recursion; part = UIDrawPart
            void FlushDeferredUIText();
            void DrawUI(UI_State* ui);
            // Draws a UI context straight onto the window, over whatever is there.
            // Call after OnRender; for tools whose UI covers the whole window.
            // standalone: no scene was rendered this frame (loading screens).
            void ScreenUIPass(UI_State* ui, bool standalone = false);

            // Post Process
            void ToneMappingPass();
            void BloomPass();
            void FXAAPass();
            void SMAAPass();
            void DebandingPass();
            void ChromaticAberationPass();
            void EyeAdaptationPass();
            void FilmicGrainPass();
            void OutlinePass();
            void DepthOfFieldPass();
            void SharpenPass();
            void VignettePass();
            void MotionBlurPass();
            void SSRPass();
            void TAAPass();
            void ContactShadowPass();
            void VolumetricFogPass();
            void GIPass();

            void UIPass();

            float SubmitTexture(Texture* texture);
            float SubmitParticleTexture(Texture* texture);
            void UpdateCascades(Scene* scene, Light* light);

            Texture2D* GetMainTexture() const { return m_MainTexture; }

            bool m_DebugRenderEnabled = false;
            bool m_EnableUIPass       = true;
            struct LUMOS_EXPORT RenderCommand2D
            {
                Renderable2D* renderable = nullptr;
                Mat4 transform;
            };

            typedef TDArray<RenderCommand2D> CommandQueue2D;

            struct Render2DLimits
            {
                uint32_t MaxQuads          = 1000;
                uint32_t QuadsSize         = RENDERER2D_VERTEX_SIZE * 4;
                uint32_t BufferSize        = 1000 * RENDERER2D_VERTEX_SIZE * 4;
                uint32_t IndiciesSize      = 1000 * 6;
                uint32_t MaxTextures       = 16;
                uint32_t MaxBatchDrawCalls = 500;

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
                float m_CascadeFade;
                float m_InitialBias;
                float CascadeFarPlaneOffset = 50.0f, CascadeNearPlaneOffset = -50.0f;
                CommandQueue m_CascadeCommandQueue[SHADOWMAP_MAX];

                TextureDepthArray* m_ShadowTex;
                uint32_t m_ShadowMapNum;
                uint32_t m_ShadowMapSize;
                bool m_ShadowMapsInvalidated;
                Mat4 m_ShadowProjView[SHADOWMAP_MAX];
                Vec4 m_SplitDepth = Vec4(-10.0f, -50.0f, -150.0f, -500.0f);
                Mat4 m_LightMatrix;
                TDArray<SharedPtr<Graphics::DescriptorSet>> m_DescriptorSet;

                SharedPtr<Shader> m_Shader              = nullptr;
                SharedPtr<Shader> m_ShaderAlpha         = nullptr;
                SharedPtr<Shader> m_ShaderAnim          = nullptr;
                SharedPtr<Shader> m_ShaderAnimAlpha     = nullptr;
                SharedPtr<Shader> m_ShaderInstanced     = nullptr;
                SharedPtr<Shader> m_ShaderInstancedAlpha = nullptr;

                TDArray<StorageBuffer*> m_InstanceTransformSSBO;
                TDArray<SharedPtr<DescriptorSet>> m_InstanceDescriptorSet;

                Maths::Frustum m_CascadeFrustums[SHADOWMAP_MAX];
                float m_CascadeTexelWorld[SHADOWMAP_MAX] = {}; // world size of one shadow texel per cascade

                // Spot light shadows: square tiles packed into the array layers after the cascades
                struct SpotShadow
                {
                    Mat4 ProjView;
                    Maths::Frustum Frustum;
                    u64 LightID   = 0;  // entity handle, keys the redraw cache
                    u64 Hash      = 0;  // what is in the tile: matrix + casters
                    u32 Layer     = 0;  // atlas layer (array layer = m_ShadowMapNum + Layer)
                    u32 X = 0, Y  = 0;  // tile corner, pixels
                    u32 Size      = 0;  // tile size, pixels
                    float Bias       = 1.0f;
                    float NormalBias = 1.0f;
                    float Fade       = 1.0f;
                    float TexelWorld = 0.0f; // world size of one texel 1 m from the light
                    float Score      = 0.0f;
                    i32 LightIndex   = -1;   // into this frame's light list
                    bool OnChange    = false;
                    bool Draw        = true; // false = last frame's tile is still right
                };
                // a spot light asking for a shadow, before the cut to MaxSpotShadows
                struct SpotCandidate
                {
                    const Light* Source = nullptr;
                    u64 ID         = 0;
                    u32 Index      = 0;
                    float Distance = 0.0f;
                    float Score    = 0.0f;
                    float Far      = 0.0f;
                    u32 Size       = 0;
                };
                SpotShadow m_Spots[SPOT_SHADOW_MAX];
                CommandQueue m_SpotCommandQueue[SPOT_SHADOW_MAX];
                u32 m_SpotCount  = 0;
                u32 m_SpotLayers = 0; // atlas layers the array was made with
            };

            static constexpr uint32_t MAX_INSTANCE_COUNT = 4096;

            struct ForwardData
            {
                Texture2D* m_DefaultTexture;
                Material* m_DefaultMaterial;

                UniquePtr<Texture2D> m_BRDFLUT;
                TDArray<Lumos::Graphics::CommandBuffer*> m_CommandBuffers;

                Mat4 m_BiasMatrix;
                Texture* m_EnvironmentMap = nullptr;
                Texture* m_IrradianceMap  = nullptr;

                CommandQueue m_CommandQueue;

                TDArray<SharedPtr<Graphics::DescriptorSet>> m_DescriptorSet;

                SharedPtr<Shader> m_Shader           = nullptr;
                SharedPtr<Shader> m_AnimShader       = nullptr;
                SharedPtr<Shader> m_InstancedShader  = nullptr;
                Texture* m_RenderTexture             = nullptr;
                TextureDepth* m_DepthTexture         = nullptr;

                Maths::Frustum m_Frustum;

                uint32_t m_RenderMode      = 0;
                uint32_t m_CurrentBufferID = 0;
                bool m_DepthTest           = false;
                size_t m_DynamicAlignment;
                Mat4* m_TransformData = nullptr;

                // Ring-buffered per frame-in-flight (see ShadowData note).
                TDArray<StorageBuffer*> m_InstanceTransformSSBO;
                TDArray<SharedPtr<DescriptorSet>> m_InstanceDescriptorSet;

            };

            struct Renderer2DData
            {
                CommandQueue2D m_CommandQueue2D;
                TDArray<TDArray<VertexBuffer*>> m_VertexBuffers;

                uint32_t m_BatchDrawCallIndex = 0;
                uint32_t m_IndexCount         = 0;

                Render2DLimits m_Limits;

                IndexBuffer* m_IndexBuffer = nullptr;
                VertexData* m_Buffer       = nullptr;

                TDArray<Mat4> m_TransformationStack;
                const Mat4* m_TransformationBack {};

                Texture* m_Textures[MAX_BOUND_TEXTURES];
                uint32_t m_TextureCount = 0;

                uint32_t m_CurrentBufferID = 0;
                Vec3 m_QuadPositions[4];

                bool m_RenderToDepthTexture;
                bool m_TriangleIndicies = false;

                TDArray<uint32_t> m_PreviousFrameTextureCount;
                SharedPtr<Shader> m_Shader     = nullptr;
                SharedPtr<Pipeline> m_Pipeline = nullptr;

                TDArray<TDArray<SharedPtr<Graphics::DescriptorSet>>> m_DescriptorSet;
            };

            struct DebugDrawData
            {
                TDArray<TDArray<Graphics::VertexBuffer*>> m_LineVertexBuffers;
                Graphics::IndexBuffer* m_LineIndexBuffer;

                Graphics::IndexBuffer* m_PointIndexBuffer = nullptr;
                TDArray<TDArray<Graphics::VertexBuffer*>> m_PointVertexBuffers;

                TDArray<SharedPtr<Graphics::DescriptorSet>> m_LineDescriptorSet;
                TDArray<SharedPtr<Graphics::DescriptorSet>> m_PointDescriptorSet;

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
            SceneRendererStats& GetSceneRendererStats() { return m_Stats; }

            void CreateCubeMap(const std::string& filePath, const Vec4& params, SharedPtr<TextureCube>& outEnv, SharedPtr<TextureCube>& outIrr);
            // mips + GGX prefilter of env into filtered, cosine irradiance into irr (recorded, not submitted)
            void FilterCubeMap(CommandBuffer* commandBuffer, TextureCube* env, TextureCube* filtered, TextureCube* irr);

            // Environment probes: the scene seen from pos over the next six frames (one cube face each),
            // filtered and used as the image based lighting in place of the Environment's sky - real
            // reflections and a directional ambient from what is actually around. The probe nearest the
            // camera lights the frame. index replaces an existing probe (-1 = add). Returns the index.
            int CaptureEnvironmentProbe(const Vec3& pos, u32 size = 128, float gain = 1.0f, int index = -1);
            void ClearEnvironmentProbes();
            bool IsCapturingProbe() const { return m_Probe.Face >= 0 || m_Probe.PendingFilter; }
            u32 GetEnvironmentProbeCount() const { return (u32)m_Probes.Size(); }

            void SetDisablePostProcess(bool disabled) { m_DisablePostProcess = disabled; }

            // The renderer drawing the game the player sees: the application's own, or the
            // editor's Game view once it has registered one. Debug tools act on this.
            static SceneRenderer* GetGameRenderer();
            static void SetGameRenderer(SceneRenderer* renderer);

            // Stop the frame after this pass (-1 = render everything). UI and Final always run.
            void SetPassCutoff(i32 pass) { m_PassCutoff = pass < (i32)RenderPass_Count ? pass : -1; }
            i32 GetPassCutoff() const { return m_PassCutoff; }
            // Tone map a frame cut before the tone mapping pass so it isn't shown as raw HDR
            void SetPassPreviewTonemap(bool enabled) { m_PassPreviewTonemap = enabled; }
            bool GetPassPreviewTonemap() const { return m_PassPreviewTonemap; }
            const RenderPassRecord& GetPassRecord(RenderPassID id) const { return m_PassRecords[id]; }

        private:
            void InitDebugRenderData();
            bool m_DebugRenderDataInitialised = false;

            bool BeginPass(RenderPassID id);
            void EndPass(RenderPassID id);
            i32 m_PassCutoff          = -1;
            bool m_PassPreviewTonemap = true;
            u32 m_PassStartDraws      = 0;
            RenderPassRecord m_PassRecords[RenderPass_Count];

            void Init2DRenderData();
            bool m_2DRenderDataInitialised = false;

            Texture2D* m_MainTexture         = nullptr;
            Texture2D* m_ResolveTexture      = nullptr;
            Texture2D* m_LastRenderTarget    = nullptr;
            Texture2D* m_PostProcessTexture1 = nullptr;

            Camera* m_Camera                    = nullptr;
            Maths::Transform* m_CameraTransform = nullptr;

            Camera* m_OverrideCamera                    = nullptr;
            Maths::Transform* m_OverrideCameraTransform = nullptr;

            struct ProbeData
            {
                int Face           = -1; // face being captured this frame, -1 = none
                bool PendingFilter = false;
                int Index          = 0;  // the stored probe the capture fills
                Vec3 Position;
                u32 Size   = 128;
                float Gain = 1.0f; // scales what the faces saw: stands in for the bounces one capture misses
                Mat4 ProjView;
                Camera* Cam             = nullptr;
                Maths::Transform* Trans = nullptr;
                SharedPtr<TextureCube> Cube;
                SharedPtr<DescriptorSet> Set;
            } m_Probe;
            struct StoredProbe
            {
                Vec3 Position;
                SharedPtr<TextureCube> Filtered, Irradiance;
            };
            TDArray<StoredProbe> m_Probes;
            int m_ActiveProbe = -1;
            void ProbeFacePass();
            void FinishProbe();
            Maths::Frustum* m_OverrideFrustum           = nullptr;

            ShadowData m_ShadowData;
            void PlanSpotShadows(Scene* scene, ShadowData::SpotCandidate* candidates, u32 count, GPULight* lights);
            ForwardData m_ForwardData;
            Renderer2DData m_Renderer2DData;
            Renderer2DData m_TextRendererData;
            DebugDrawData m_DebugDrawData;
            Renderer2DData m_DebugTextRendererData;
            Renderer2DData m_ParticleData;
            Renderer2DData m_LitData; // forward-lit 2D sprites

            TextVertexData* TextVertexBufferPtr = nullptr;

            // Forward 2D light data, packed for the Batch2DLit lights UBO (std140)
            static const int MAX_2D_LIGHTS = 32;
            struct Light2DGPU
            {
                Vec4 Colour;    // rgb
                Vec4 Position;  // xy world, w radius
                Vec4 Direction; // xy spot dir, z height, w type
                Vec4 Params;    // x intensity, y innerAngle, z outerAngle, w falloff
            };
            struct Lights2DUBO
            {
                Light2DGPU lights[MAX_2D_LIGHTS];
                Vec4 ambient;
                int counts[4];
            };
            Lights2DUBO m_Lights2D;
            bool m_LitDataInitialised = false;

            // Vertex data per frame in flight, per batch
            TDArray<TDArray<VertexData*>> m_ParticleBufferBase;
            TDArray<TDArray<VertexData*>> m_2DBufferBase;
            TDArray<TDArray<VertexData*>> m_LitBufferBase;
            TDArray<TDArray<LineVertexData*>> m_LineBufferBase;
            TDArray<TDArray<PointVertexData*>> m_PointBufferBase;
            TDArray<TDArray<VertexData*>> m_QuadBufferBase;
            TDArray<TextVertexData*> TextVertexBufferBase;
            TDArray<TextVertexData*> DebugTextVertexBufferBase;
            TextVertexData* DebugTextVertexBufferPtr = nullptr;

            Vec4 m_ClearColour;

            int m_ToneMapIndex     = 4;
            float m_Exposure       = 1.0f;
            float m_BloomIntensity = 1.0f;
            Scene* m_CurrentScene  = nullptr;
            bool m_GenerateBRDFLUT = false;
            bool m_SupportCompute  = false;
            bool m_ForwardPlusEnabled = false;

            // Forward+ light culling
            SharedPtr<Graphics::Shader> m_LightCullingShader;
            SharedPtr<Graphics::DescriptorSet> m_LightCullingDescriptorSet;
            Graphics::StorageBuffer* m_LightSSBO      = nullptr;
            Graphics::StorageBuffer* m_LightGridSSBO  = nullptr;
            Graphics::StorageBuffer* m_LightIndexSSBO = nullptr;
            Graphics::StorageBuffer* m_GlobalIndexCountSSBO = nullptr;
            uint32_t m_TileSize   = 16;
            uint32_t m_TileCountX = 0;
            uint32_t m_TileCountY = 0;
            uint32_t m_NumLights  = 0;
            bool m_HasLocalLights = false;    // any point / spot light this frame
            uint32_t m_StaticGridLights = ~0u; // light count the CPU-written tile lists hold; ~0 = the compute pass wrote them
            uint32_t m_StaticGridWrites = 0;   // frames left to rewrite them (a frame still in flight may run the compute)

            // Temp
            bool m_DisablePostProcess = false;
            bool m_TransparentBackground = false;

            // Targets only some scenes need start at 1x1 and grow the first time their pass runs,
            // so a UI-only app doesn't carry shadow maps, G-buffers or bloom chains
            void EnsureGBufferTargets();
            void EnsureBloomTargets();
            void EnsureSMAATargets();
            void EnsureResolveTarget();
            void EnsureShadowTargets();
            bool m_GBufferAllocated = false;
            bool m_BloomAllocated   = false;
            bool m_SMAAAllocated    = false;
            bool m_ResolveAllocated = false;
            bool m_ShadowsAllocated = false;
            bool m_LightIndexAllocated = false; // Forward+ per-tile light lists
            bool m_Has3D            = false; // anything in the forward queue this frame

            // Occlusion culling: a coarse max-depth grid of the depth pre-pass is read back two
            // frames later (GPU never stalls) and meshes whose box sits behind it skip the forward queue.
            struct OcclusionData
            {
                static constexpr u32 GridX     = 128;
                static constexpr u32 GridY     = 72;
                static constexpr u32 MaxLevels = 8;

                SharedPtr<Graphics::Shader> Shader;
                TDArray<StorageBuffer*> Buffers;                  // per frame slot, written by the GPU
                TDArray<SharedPtr<Graphics::DescriptorSet>> Sets; // per frame slot
                TDArray<Mat4> SlotViewProj;                       // view-proj the slot's depth was drawn with
                TDArray<u8> SlotValid;
                float Storage[GridX * GridY * 4 / 3 + 64];        // CPU copy: level 0 then max-reduced levels
                float* Levels[MaxLevels] = {};
                u32 LevelW[MaxLevels]    = {};
                u32 LevelH[MaxLevels]    = {};
                u32 LevelCount           = 0;
                Mat4 TestViewProj;
                Scene* LastScene = nullptr;
                bool Active      = false;
            } m_Occlusion;

            // [0] full quality, [1] cheap (QualitySettings::LowForwardShader picks)
            SharedPtr<Graphics::Shader> m_ForwardShaders[2];
            SharedPtr<Graphics::Shader> m_InstancedShaders[2];
            void ApplyForwardQuality();

            void InitOcclusion();
            void UpdateOcclusion(const Mat4& currentViewProj, bool enabled);
            bool IsOccluded(const Maths::BoundingBox& box) const;
            float m_ViewScale       = 1.0f;
            uint32_t m_TargetWidth  = 1;
            uint32_t m_TargetHeight = 1;

            Mesh* m_ScreenQuad;
            Texture* m_CubeMap;
            Texture* m_DefaultTextureCube;
            SharedPtr<Graphics::Shader> m_SkyboxShader;
            SharedPtr<Graphics::DescriptorSet> m_SkyboxDescriptorSet;

            // Procedural nebula backdrop
            SharedPtr<Graphics::Shader> m_CosmoShader;
            SharedPtr<Graphics::DescriptorSet> m_CosmoDescriptorSet;
            SharedPtr<Graphics::Shader> m_CosmoBakeShader;
            SharedPtr<Graphics::DescriptorSet> m_CosmoBakeDescriptorSet;
            Graphics::Texture2D* m_CosmoSky[2] = { nullptr, nullptr };
            int m_CosmoSkyFront                = 0;
            bool m_CosmoSkyBothBaked           = false;
            uint32_t m_CosmoBakedVersion       = 0;
            float m_CosmoFade                  = 1.0f;
            float m_CosmoLastTime              = 0.0f;

            // PointCloud instanced-billboard pass
            SharedPtr<Graphics::Shader> m_PointCloudShader;
            SharedPtr<Graphics::DescriptorSet> m_PointCloudCameraDescriptor; // set 0 (UBO)
            SharedPtr<Graphics::DescriptorSet> m_PointCloudPointDescriptor;  // set 1 (SSBO)
            Graphics::VertexBuffer* m_PointCloudQuadVB = nullptr;
            Graphics::IndexBuffer* m_PointCloudQuadIB  = nullptr;
            bool m_PointCloudInitialised               = false;

            SharedPtr<Graphics::Shader> m_LineCloudShader;
            TDArray<SharedPtr<Graphics::DescriptorSet>> m_LineCloudCameraDescriptors; // set 0 (UBO)
            TDArray<SharedPtr<Graphics::DescriptorSet>> m_LineCloudLineDescriptors;   // set 1 (SSBO)
            Graphics::VertexBuffer* m_LineCloudQuadVB = nullptr;
            Graphics::IndexBuffer* m_LineCloudQuadIB  = nullptr;
            bool m_LineCloudInitialised               = false;

            // Planet impostor pass
            SharedPtr<Graphics::Shader> m_PlanetShader;
            SharedPtr<Graphics::DescriptorSet> m_PlanetCameraDescriptor;   // set 0 (UBO)
            SharedPtr<Graphics::DescriptorSet> m_PlanetInstanceDescriptor; // set 1 (SSBO)
            Graphics::VertexBuffer* m_PlanetQuadVB = nullptr;
            Graphics::IndexBuffer* m_PlanetQuadIB  = nullptr;
            bool m_PlanetInitialised               = false;

            SharedPtr<Graphics::Shader> m_FinalPassShader;
            SharedPtr<Graphics::Shader> m_FinalPassAlphaShader;
            SharedPtr<Graphics::DescriptorSet> m_FinalPassAlphaDescriptorSet;
            SharedPtr<Graphics::DescriptorSet> m_FinalPassDescriptorSet;

            Texture2D* m_BloomTexture               = nullptr;
            Texture2D* m_BloomTexture1              = nullptr;
            Texture2D* m_BloomTexture2              = nullptr;
            Texture2D* m_BloomTextureLastRenderered = nullptr;

            SharedPtr<Graphics::Shader> m_BloomPassShader;

            TDArray<SharedPtr<Graphics::DescriptorSet>> m_BloomDescriptorSets;

            SharedPtr<Graphics::DescriptorSet> m_FXAAPassDescriptorSet;
            SharedPtr<Graphics::Shader> m_FXAAShader;

            // SMAA 1x (3 passes: edge detect -> blend weights -> neighbourhood blend)
            SharedPtr<Graphics::Shader> m_SMAAEdgesShader;
            SharedPtr<Graphics::Shader> m_SMAAWeightsShader;
            SharedPtr<Graphics::Shader> m_SMAABlendShader;
            SharedPtr<Graphics::DescriptorSet> m_SMAAEdgesDescriptorSet;
            SharedPtr<Graphics::DescriptorSet> m_SMAAWeightsDescriptorSet;
            SharedPtr<Graphics::DescriptorSet> m_SMAABlendDescriptorSet;
            Texture2D* m_SMAAEdgesTexture   = nullptr;
            Texture2D* m_SMAAWeightsTexture = nullptr;

            SharedPtr<Graphics::DescriptorSet> m_DebandingPassDescriptorSet;
            SharedPtr<Graphics::Shader> m_DebandingShader;

            SharedPtr<Graphics::DescriptorSet> m_ChromaticAberationPassDescriptorSet;
            SharedPtr<Graphics::Shader> m_ChromaticAberationShader;

            SharedPtr<Graphics::DescriptorSet> m_DepthPrePassDescriptorSet;
            SharedPtr<Graphics::DescriptorSet> m_DepthPrePassAlphaDescriptorSet;
            SharedPtr<Pipeline> m_DepthPrePassPipeline = nullptr;
            SharedPtr<Graphics::Shader> m_DepthPrePassShader;
            SharedPtr<Graphics::Shader> m_DepthPrePassAlphaShader;
            SharedPtr<Graphics::Shader> m_DepthPrePassAnimShader;
            SharedPtr<Graphics::Shader> m_DepthPrePassInstancedShader;
            SharedPtr<Graphics::Shader> m_DepthPrePassAlphaAnimShader;
            Texture2D* m_SSAOTexture  = nullptr;
            Texture2D* m_SSAOTexture1 = nullptr;
            bool m_SSAOValid          = false;

            Texture2D* m_NoiseTexture  = nullptr;
            Texture2D* m_NormalTexture = nullptr;
            Texture2D* m_NormalResolveTexture = nullptr;

            SharedPtr<Graphics::Shader> m_SSAOShader;
            SharedPtr<Graphics::DescriptorSet> m_SSAOPassDescriptorSet;

            SharedPtr<Graphics::Shader> m_SSAOBlurShader;
            SharedPtr<Graphics::DescriptorSet> m_SSAOBlurPassDescriptorSet;
            SharedPtr<Graphics::DescriptorSet> m_SSAOBlurPassDescriptorSet2;

            SharedPtr<Graphics::Shader> m_ToneMappingPassShader;
            SharedPtr<Graphics::DescriptorSet> m_ToneMappingPassDescriptorSet;

            bool m_AdaptiveExposureReady = false;
            SharedPtr<Graphics::Shader> m_LuminanceHistogramShader;
            SharedPtr<Graphics::Shader> m_LuminanceAverageShader;
            SharedPtr<Graphics::DescriptorSet> m_LuminanceHistogramDescriptorSet;
            SharedPtr<Graphics::DescriptorSet> m_LuminanceAverageDescriptorSet;
            Graphics::StorageBuffer* m_LuminanceHistogramSSBO = nullptr;
            Graphics::StorageBuffer* m_AverageLuminanceSSBO   = nullptr;

            SharedPtr<Graphics::Shader> m_VignetteShader;
            SharedPtr<Graphics::DescriptorSet> m_VignettePassDescriptorSet;

            SharedPtr<Graphics::Shader> m_MotionBlurShader;
            SharedPtr<Graphics::DescriptorSet> m_MotionBlurPassDescriptorSet;
            Mat4 m_PrevViewProj;
            bool m_HasPrevViewProj = false;

            SharedPtr<Graphics::Shader> m_SSRShader;
            SharedPtr<Graphics::DescriptorSet> m_SSRPassDescriptorSet;

            SharedPtr<Graphics::DescriptorSet> m_FilmicGrainPassDescriptorSet;
            SharedPtr<Graphics::Shader> m_FilmicGrainShader;

            SharedPtr<Graphics::DescriptorSet> m_OutlinePassDescriptorSet;
            SharedPtr<Graphics::Shader> m_OutlineShader;

            SharedPtr<Graphics::DescriptorSet> m_DepthOfFieldPassDescriptorSet;
            SharedPtr<Graphics::Shader> m_DepthOfFieldShader;

            SharedPtr<Graphics::DescriptorSet> m_SharpenPassDescriptorSet;
            SharedPtr<Graphics::Shader> m_SharpenShader;

            // Temporal AA: BeginScene jitters the 3D camera, TAAPass blends into a history pair
            void EnsureTAATargets();
            bool TAAWanted() const;
            SharedPtr<Graphics::Shader> m_TAAShader;
            SharedPtr<Graphics::DescriptorSet> m_TAADescriptorSet;
            Texture2D* m_TAAHistory[2] = {};
            u32 m_TAAHistoryIndex      = 0;
            u32 m_TAAFrame             = 0;
            bool m_TAAAllocated        = false;
            bool m_TAAHistoryValid     = false;
            bool m_TAAJittered         = false; // this frame's 3D camera carries the jitter
            Vec2 m_TAAJitter;                   // in NDC
            Mat4 m_TAAViewProj;                 // without the jitter
            Mat4 m_TAAPrevViewProj;
            Vec3 m_TAAPrevCameraPos;

            SharedPtr<Graphics::Shader> m_ContactShadowShader;
            SharedPtr<Graphics::DescriptorSet> m_ContactShadowDescriptorSet;
            Texture2D* m_ContactShadowTexture = nullptr;
            bool m_ContactShadowsAllocated    = false;
            bool m_ContactShadowsValid        = false; // filled last frame: what this frame's forward pass may read

            SharedPtr<Graphics::Shader> m_VolumetricFogShader;
            SharedPtr<Graphics::Shader> m_VolumetricFogApplyShader;
            SharedPtr<Graphics::DescriptorSet> m_VolumetricFogDescriptorSet;
            SharedPtr<Graphics::DescriptorSet> m_VolumetricFogApplyDescriptorSet;
            Texture2D* m_FogTexture      = nullptr; // half res
            Texture2D* m_FogDepthTexture = nullptr;
            bool m_FogAllocated          = false;

            ProbeGI* m_GI = nullptr;

            // the scene's first directional light this frame (lux, before exposure)
            bool m_HasSun = false;
            Vec3 m_SunDirection;
            Vec3 m_SunIlluminance;

            SceneRendererStats m_Stats;

#ifdef LUMOS_PLATFORM_WINDOWS
            uint8_t m_MainTextureSamples = 4;
#else
            uint8_t m_MainTextureSamples = 1;
#endif

            // Outline pass
            Graphics::Model* m_SelectedModel           = nullptr;
            Maths::Transform* m_SelectedModelTransform = nullptr;

            SceneRenderSettings* m_OverrideSceneRenderSettings = nullptr; // For editor viewport

            void TextFlush(Renderer2DData& textRenderData, TDArray<TextVertexData*>& textVertexBufferBase, TextVertexData*& textVertexBufferPtr);

            bool m_CurrentUIText = false;
            Vec2 m_UIProjectionSize = Vec2(0.0f, 0.0f);
            bool m_UISwapchainTarget = false;
            TDArray<UI_Widget*>* m_UIDeferredText = nullptr;
        };
    }
}
