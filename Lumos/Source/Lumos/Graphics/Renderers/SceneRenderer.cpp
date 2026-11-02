#include "Precompiled.h"
#include "SceneRenderer.h"
#include "Scene/Entity.h"
#include "Scene/Component/ModelComponent.h"
#include "Scene/Component/VoxelWorldComponent.h"
#include "Graphics/Model.h"
#include "Graphics/Animation/Skeleton.h"
#include "Graphics/Renderers/IRenderer.h"
#include "Graphics/Renderers/DebugRenderer.h"
#include "Graphics/Renderers/ProbeGI.h"
#include "Scene/Component/DecalComponent.h"
#include "Graphics/Light.h"
#include "Graphics/Camera/Camera.h"
#include "Graphics/Environment.h"
#include "Graphics/Material.h"
#include "Graphics/Mesh.h"
#include "Graphics/Sprite.h"
#include "Graphics/AnimatedSprite.h"
#include "Graphics/Light2D.h"
#include "Graphics/RHI/GPUProfile.h"
#include "Graphics/RHI/VertexBuffer.h"
#include "Graphics/RHI/IndexBuffer.h"
#include "Graphics/PointCloud.h"
#include "Graphics/LineCloud.h"
#include "Graphics/RHI/StorageBuffer.h"
#include "Graphics/RHI/Texture.h"
#include "Graphics/Font.h"
#include "Graphics/WorldText.h"
#include "Graphics/PlanetField.h"
#include "Graphics/CosmoBackdrop.h"
#include "Graphics/MSDFData.h"
#include "Graphics/ParticleManager.h"
#include "Graphics/Animation/AnimationController.h"
#include "Core/JobSystem.h"
#include "Core/OS/Window.h"
#include "Core/Algorithms/Sort.h"
#include "Maths/BoundingSphere.h"
#include "Maths/BoundingBox.h"
#include "Maths/Rect.h"
#include "Maths/MathsUtilities.h"
#include "Events/ApplicationEvent.h"

#include "Embedded/EmbeddedAssetData.h"
#include "Embedded/CheckerBoardTextureArray.inl"
#include "Core/Asset/AssetManager.h"
#include "Core/Application.h"
#include "Core/OS/OS.h"
#include "Scene/Component/Components.h"
#include "Maths/Random.h"
#include "ImGui/ImGuiUtilities.h"
#include "Graphics/UI.h"
#include "Core/Engine.h"
#include "Utilities/Timer.h"

#include <imgui/imgui.h>

#include <cmath>

static const uint32_t MaxPoints                  = 1000;
static const uint32_t MaxPointVertices           = MaxPoints * 4;
static const uint32_t MaxPointIndices            = MaxPoints * 6;
static const uint32_t MAX_BATCH_DRAW_CALLS       = 100;
static const uint32_t RENDERER_POINT_SIZE        = sizeof(Lumos::Graphics::PointVertexData) * 4;
static const uint32_t RENDERER_POINT_BUFFER_SIZE = RENDERER_POINT_SIZE * MaxPointVertices;

static const uint32_t MaxLines                  = 1000;
static const uint32_t MaxLineVertices           = MaxLines * 2;
static const uint32_t MaxLineIndices            = MaxLines * 2;
static const uint32_t MAX_LINE_BATCH_DRAW_CALLS = 100;
static const uint32_t RENDERER_LINE_SIZE        = sizeof(Lumos::Graphics::LineVertexData) * 4;
static const uint32_t RENDERER_LINE_BUFFER_SIZE = RENDERER_LINE_SIZE * MaxLineVertices;
static const uint32_t MAX_LIGHTS                = 256;
// batch vertex buffers start here and grow in powers of two: a UI makes many tiny batches
static const uint32_t BATCH_BUFFER_INITIAL      = 16 * 1024;
static const uint32_t FORWARD_PLUS_TILE_SIZE    = 16;
static const uint32_t MAX_LIGHTS_PER_TILE       = 256;

namespace Lumos::Graphics
{
    // Batch2D.frag linearises texture colours for the scene; slots offset by this are drawn as they are
    static constexpr float kUITextureRaw = 100.0f;

    enum UIDrawPart
    {
        UIDrawPart_All = 0,
        UIDrawPart_Quads,
        UIDrawPart_Text,
    };

#ifdef LUMOS_RENDER_API_VULKAN
    void TransitionImageToCorrectLayout(Texture* texture, CommandBuffer* cmdBuffer); // VKDescriptorSet.cpp
#endif
    static TimeStamp s_PassStartTime;
    // Cascades are fitted to MaxShadowDistance * kShadowFarScale, and the last one ends at kLastCascadeSplit of that
    // ---- procedural sky ----
    // The sky's sun is the scene's directional light. Radiance is kept in the units the
    // environment maps use (cd/m2 / 120000), so the picture and the lighting share a scale.
    static constexpr float kSkyUnit      = 120000.0f;
    static constexpr float kSkyMinSunLux = 1000.0f;
    static constexpr u32 kSkyGridX       = 128;
    static constexpr u32 kSkyGridY       = 72;

    // ---- cloud noise ----
    // The clouds' shapes, worked out once into a texture that tiles: hashing noise per pixel
    // was most of a frame. r = base shape (5 octaves, 8 cells across), g b = the slow warp,
    // a = fine detail. Sky.glslh reads it as SKY_NOISE at uv / 8.
    static Graphics::Texture2D* s_SkyNoiseTexture = nullptr;

    static float SkyLatticeHash(u32 x, u32 y, u32 salt)
    {
        u32 h = x * 374761393u + y * 668265263u + salt * 2246822519u;
        h     = (h ^ (h >> 13)) * 1274126177u;
        h ^= h >> 16;
        return (float)(h & 0xFFFFFFu) / (float)0xFFFFFF;
    }

    // Gradient noise that repeats every `period` cells, 0..1.
    static float SkyPeriodicNoise(float x, float y, u32 period, u32 salt)
    {
        const float fx = floorf(x), fy = floorf(y);
        const u32 ix = (u32)((int)fx % (int)period + (int)period) % period;
        const u32 iy = (u32)((int)fy % (int)period + (int)period) % period;
        const float tx = x - fx, ty = y - fy;
        const float ux = tx * tx * tx * (tx * (tx * 6.0f - 15.0f) + 10.0f);
        const float uy = ty * ty * ty * (ty * (ty * 6.0f - 15.0f) + 10.0f);
        auto corner    = [&](u32 cx, u32 cy, float dx, float dy)
        {
            const u32 px = (ix + cx) % period, py = (iy + cy) % period;
            const float gx = SkyLatticeHash(px, py, salt) * 2.0f - 1.0f;
            const float gy = SkyLatticeHash(px, py, salt + 17u) * 2.0f - 1.0f;
            return gx * dx + gy * dy;
        };
        const float a = corner(0, 0, tx, ty), b = corner(1, 0, tx - 1.0f, ty);
        const float c = corner(0, 1, tx, ty - 1.0f), d = corner(1, 1, tx - 1.0f, ty - 1.0f);
        const float n = (a + (b - a) * ux) + ((c + (d - c) * ux) - (a + (b - a) * ux)) * uy;
        return n * 0.5f + 0.5f;
    }

    static float SkyPeriodicFBM(float x, float y, u32 period, u32 octaves, u32 salt)
    {
        float total = 0.0f, amp = 0.5f, norm = 0.0f;
        for(u32 i = 0; i < octaves; i++)
        {
            total += amp * SkyPeriodicNoise(x, y, period, salt + i * 101u);
            norm += amp;
            x *= 2.0f;
            y *= 2.0f;
            period *= 2;
            amp *= 0.5f;
        }
        return total / norm * 0.96875f;
    }

    static Graphics::Texture2D* GetSkyNoiseTexture()
    {
        if(s_SkyNoiseTexture)
            return s_SkyNoiseTexture;

        const u32 size    = 256;
        ArenaTemp scratch = ScratchBegin(nullptr, 0);
        u8* pixels        = PushArrayNoZero(scratch.arena, u8, size * size * 4);
        for(u32 y = 0; y < size; y++)
        {
            for(u32 x = 0; x < size; x++)
            {
                // the texture spans 8 units of cloud uv
                const float u = (float)x / (float)size * 8.0f;
                const float v = (float)y / (float)size * 8.0f;
                const float values[4] = {
                    SkyPeriodicFBM(u, v, 8, 5, 1u),
                    SkyPeriodicNoise(u * 0.5f, v * 0.5f, 4, 301u),
                    SkyPeriodicNoise(u * 0.5f, v * 0.5f, 4, 577u),
                    SkyPeriodicFBM(u * 3.0f, v * 3.0f, 24, 3, 811u),
                };
                for(u32 c = 0; c < 4; c++)
                    pixels[(y * size + x) * 4 + c] = (u8)Maths::Clamp(values[c] * 255.0f + 0.5f, 0.0f, 255.0f);
            }
        }

        Graphics::TextureDesc desc = {};
        desc.format                = Graphics::RHIFormat::R8G8B8A8_Unorm;
        desc.wrap                  = TextureWrap::REPEAT;
        desc.minFilter             = TextureFilter::LINEAR;
        desc.magFilter             = TextureFilter::LINEAR;
        desc.generateMipMaps       = true;
        desc.anisotropicFiltering  = false;
        desc.srgb                  = false;
        desc.flags                 = 0;
        s_SkyNoiseTexture          = Graphics::Texture2D::CreateFromSource(size, size, (void*)pixels, desc);
        ScratchEnd(scratch);
        return s_SkyNoiseTexture;
    }

    // ---- shadow cascades ----
    // Every cascade is redrawn every frame. One with nothing in it is cleared once, then left.
    // A spot tile drawn on an earlier frame, so a light set to redraw on change can skip it
    struct SpotTileCache
    {
        u64 LightID = 0;
        u64 Hash    = 0;
        u32 Layer = 0, X = 0, Y = 0, Size = 0;
        bool Valid = false;
    };

    struct CascadeSchedule
    {
        SceneRenderer* Owner = nullptr;
        u32 MapSize          = 0;
        bool Skip[SHADOWMAP_MAX]       = {};
        bool EmptyDrawn[SHADOWMAP_MAX] = {};
        TextureDepth* Cascades[SHADOWMAP_MAX + SPOT_SHADOW_LAYERS] = {}; // copy path only, one per array layer
        SpotTileCache Spots[SPOT_SHADOW_MAX];
    };
    static CascadeSchedule s_CascadeSchedules[4];

    static CascadeSchedule& CascadeScheduleFor(SceneRenderer* renderer)
    {
        for(CascadeSchedule& entry : s_CascadeSchedules)
            if(entry.Owner == renderer)
                return entry;
        for(CascadeSchedule& entry : s_CascadeSchedules)
            if(!entry.Owner)
            {
                entry.Owner = renderer;
                return entry;
            }
        s_CascadeSchedules[0]       = CascadeSchedule();
        s_CascadeSchedules[0].Owner = renderer;
        return s_CascadeSchedules[0];
    }
    // Intel GPUs on macOS (MoltenVK) draw wrong into a single layer of a depth array, so there
    // each cascade is drawn into its own texture and copied into its layer.
    // LUMOS_SHADOW_COPY=0/1 forces it either way.
    static bool ShadowCopyPath()
    {
        static int s_Copy = -1;
        if(s_Copy < 0)
        {
            const char* env = getenv("LUMOS_SHADOW_COPY");
            if(env)
                s_Copy = atoi(env) != 0 ? 1 : 0;
            else
            {
#if defined(LUMOS_PLATFORM_MACOS)
                s_Copy = Renderer::GetCapabilities().Vendor == "Intel" ? 1 : 0;
#else
                s_Copy = 0;
#endif
            }
        }
        return s_Copy == 1;
    }

    static void SetShadowTarget(PipelineDesc& desc, SceneRenderer* renderer, TextureDepthArray* array, u32 layer, u32 size)
    {
        // light matrices are standard-Z: shadow maps clear to 1 and keep the nearest with LESS
        desc.ReverseDepth = false;
        if(!ShadowCopyPath())
        {
            desc.depthArrayTarget = reinterpret_cast<Texture*>(array);
            desc.depthTarget      = nullptr;
            return;
        }

        TextureDepth*& target = CascadeScheduleFor(renderer).Cascades[layer];
        if(!target)
            target = TextureDepth::Create(size, size, Renderer::GetRenderer()->GetDepthFormat(), 1);
        else if(target->GetWidth() != size)
            target->Resize(size, size);
        desc.depthArrayTarget = nullptr;
        desc.depthTarget      = target;
    }

    // ---- spot light shadows ----
    // How far a spot's shadow reaches. Without LightRange the light never quite ends, so stop where it has faded.
    static float SpotShadowFar(const Light& light, bool lightRange)
    {
        const float radius = Maths::Max(light.Radius, 0.1f);
        return lightRange ? radius : radius * 10.0f;
    }

    static float SpotShadowHalfAngle(const Light& light)
    {
        // the shader's cone: cos(half angle) = 1 - Angle
        return acosf(Maths::Clamp(1.0f - light.Angle, -1.0f, 1.0f));
    }

    static u32 SpotShadowTileSize(const Light& light, float farPlane, float distance, int maxResolution, u32 mapSize, float tanHalfFov, float screenHeight)
    {
        const u32 cap = Maths::Min((u32)Maths::Max(maxResolution, 256), mapSize);
        u32 size      = 0;
        if(light.ShadowResolution > 0)
            size = 256u << Maths::Min((u32)light.ShadowResolution - 1u, 3u);
        else if(tanHalfFov <= 0.0f)
            size = cap;
        else
        {
            // about one texel per pixel of the lit cone's footprint on screen
            const float coneRadius = farPlane * tanf(Maths::Min(SpotShadowHalfAngle(light), 1.4f));
            const float pixels     = screenHeight * coneRadius / (Maths::Max(distance, 0.5f) * tanHalfFov);
            size                   = 256;
            while(size < (u32)pixels && size < cap)
                size <<= 1;
        }
        return Maths::Min(Maths::Max(size, 256u), cap);
    }

    static u32 MortonCompact(u32 v)
    {
        v &= 0x55555555u;
        v = (v | (v >> 1)) & 0x33333333u;
        v = (v | (v >> 2)) & 0x0f0f0f0fu;
        v = (v | (v >> 4)) & 0x00ff00ffu;
        v = (v | (v >> 8)) & 0x0000ffffu;
        return v;
    }

    static u64 HashBytes(u64 hash, const void* data, size_t size)
    {
        const u8* bytes = (const u8*)data;
        for(size_t i = 0; i < size; i++)
            hash = (hash ^ bytes[i]) * 1099511628211ull;
        return hash;
    }

    static bool s_CullDepthValid    = false; // light culling: the depth buffer holds this frame

    // Every pipeline drawing with a camera is reversed-Z (near = 1, far = 0, GREATER_OR_EQUAL):
    // projections the renderer makes itself have to match.
    static Mat4 ReverseZ(Mat4 ortho)
    {
        ortho.values[10] = -ortho.values[10];
        ortho.values[14] = 1.0f - ortho.values[14];
        return ortho;
    }

    static Mat4 ReversedPerspective(float nearPlane, float farPlane, float aspect, float fov)
    {
        Mat4 m      = Mat4::Perspective(nearPlane, farPlane, aspect, fov);
        float range = Maths::Max(farPlane - nearPlane, 1e-6f);
        m.values[10] = nearPlane / range;
        m.values[14] = farPlane * nearPlane / range;
        return m;
    }

    // A panorama sky drawn from the lighting cube is only EnvironmentMapSize per face (128 on
    // low quality) - blocky. The skybox gets its own unfiltered cube at the panorama's detail,
    // found again by the lighting cube it was baked with.
    struct SkyboxCube
    {
        Texture* Lighting = nullptr;
        SharedPtr<TextureCube> Sky;
    };
    static SkyboxCube s_SkyboxCubes[4];
    static u32 s_SkyboxCubeNext = 0;

    static Texture* SkyboxCubeFor(Texture* lighting)
    {
        for(SkyboxCube& entry : s_SkyboxCubes)
            if(lighting && entry.Lighting == lighting && entry.Sky)
                return entry.Sky.get();
        return lighting;
    }

    static Vec3 s_SkySunDirection   = Vec3(0.0f, 0.6f, 0.8f);
    static Vec3 s_SkySunIlluminance = Vec3(1.0f); // lux, rgb
    static bool s_SkySunFromLight   = false;

    // What the lighting cube map is baked from. Compared to decide when it is out of date.
    struct SkyBakeData
    {
        Vec4 Parameters;
        Vec4 SunDirection; // xyz towards the sun
        Vec4 SunColour;    // rgb illuminance / kSkyUnit, w time
        Vec4 HorizonTint;
        Vec4 ZenithTint;
        Vec4 CloudColour;
        Vec4 CloudParams;
        Vec4 CloudWindDir;
    };
    static SkyBakeData s_SkyBaked       = {};
    static u32 s_SkyFramesSinceBake     = 0;
    // The sky as the last frame saw it. A bake can be asked for while a scene is still being
    // read in (its registry is not safe to look through then), so it works from this.
    static SkyBakeData s_SkyLatest      = {};
    static bool s_SkyLatestValid        = false;

    // The horizon / zenith colours predate the scattering model: they are read as a tint
    // against the old defaults, so an untouched scene gets the sky as computed.
    static Vec4 SkyTint(const Vec4& colour, const Vec3& neutral)
    {
        return Vec4(Maths::Clamp(colour.x / neutral.x, 0.0f, 8.0f), Maths::Clamp(colour.y / neutral.y, 0.0f, 8.0f), Maths::Clamp(colour.z / neutral.z, 0.0f, 8.0f), 1.0f);
    }

    static SkyBakeData BuildSkyBake(Scene* scene, const Vec4& parameters)
    {
        SkyBakeData data;
        data.Parameters   = parameters;
        data.HorizonTint  = Vec4(1.0f);
        data.ZenithTint   = Vec4(1.0f);
        data.CloudColour  = Vec4(1.0f, 1.0f, 1.0f, 0.45f);
        data.CloudParams  = Vec4(0.5f, 0.7f, 0.02f, -1.0f);
        data.CloudWindDir = Vec4(1.0f, 0.0f, 0.2f, 0.5f);

        Vec3 direction   = s_SkySunDirection;
        Vec3 illuminance = s_SkySunIlluminance;
        bool fromLight   = s_SkySunFromLight;

        if(scene)
        {
            auto view = scene->GetRegistry().view<Graphics::Environment>();
            if(!view.empty())
            {
                const auto& env   = view.get<Graphics::Environment>(view.front());
                data.HorizonTint  = SkyTint(env.GetHorizonColour(), Vec3(0.66f, 0.78f, 0.92f));
                data.ZenithTint   = SkyTint(env.GetZenithColour(), Vec3(0.13f, 0.34f, 0.78f));
                data.CloudColour  = env.GetCloudColour();
                data.CloudParams  = env.GetCloudParams();
                data.CloudWindDir = env.GetCloudWindDir();
                // no directional light: the environment's own sun, w = strength of a midday one
                if(!fromLight)
                {
                    const Vec4& sun = env.GetSunDirection();
                    direction       = Vec3(sun.x, sun.y, sun.z);
                    illuminance     = Vec3(100000.0f * sun.w);
                }
            }
        }

        if(direction.LengthSquared() < 1.0e-6f)
            direction = Vec3(0.0f, 1.0f, 0.0f);
        direction.Normalise();
        data.SunDirection = Vec4(direction, 0.0f);
        data.SunColour    = Vec4(illuminance / kSkyUnit, (float)Engine::GetTimeStep().GetElapsedSeconds());
        return data;
    }

    // Worth another bake? (Time alone is not: the clouds in the lighting can lag.)
    static bool SkyBakeChanged(const SkyBakeData& a, const SkyBakeData& b)
    {
        const Vec3 da(a.SunDirection.x, a.SunDirection.y, a.SunDirection.z);
        const Vec3 db(b.SunDirection.x, b.SunDirection.y, b.SunDirection.z);
        if(Maths::Dot(da, db) < 0.99939f) // 2 degrees
            return true;
        const float ea = a.SunColour.x + a.SunColour.y + a.SunColour.z;
        const float eb = b.SunColour.x + b.SunColour.y + b.SunColour.z;
        if(Maths::Abs(ea - eb) > 0.04f * Maths::Max(ea, eb) + 1.0e-5f)
            return true;
        const Vec4* pa[] = { &a.SunColour, &a.HorizonTint, &a.ZenithTint, &a.CloudColour, &a.CloudParams, &a.CloudWindDir };
        const Vec4* pb[] = { &b.SunColour, &b.HorizonTint, &b.ZenithTint, &b.CloudColour, &b.CloudParams, &b.CloudWindDir };
        for(int i = 0; i < 6; i++)
        {
            // the sun's w is the time
            if(Maths::Abs(pa[i]->x - pb[i]->x) > 0.01f || Maths::Abs(pa[i]->y - pb[i]->y) > 0.01f || Maths::Abs(pa[i]->z - pb[i]->z) > 0.01f)
                return true;
            if(i > 0 && Maths::Abs(pa[i]->w - pb[i]->w) > 0.01f)
                return true;
        }
        return false;
    }

    static constexpr float kShadowFarScale   = 1.2f;
    static constexpr float kLastCascadeSplit = 0.35f;

    static void UploadBatch(VertexBuffer* vb, uint32_t size, const void* data)
    {
        if(size > vb->GetSize())
        {
            uint32_t capacity = BATCH_BUFFER_INITIAL;
            while(capacity < size)
                capacity *= 2;
            vb->Resize(capacity);
        }
        vb->SetData(size, data, true);
    }

    struct ForwardInstanceData
    {
        Mat4 transform;
        Vec4 colour;
    };

    static u64 InstanceMaterialKey(Material* m)
    {
        if(!m)
            return 0;
        u64 h    = 1469598103934665603ull;
        auto mix = [&](u64 v)
        { h ^= v; h *= 1099511628211ull; };

        const auto& t = m->GetTextures();
        mix((u64)(uintptr_t)t.albedo.get());
        mix((u64)(uintptr_t)t.normal.get());
        mix((u64)(uintptr_t)t.metallic.get());
        mix((u64)(uintptr_t)t.roughness.get());
        mix((u64)(uintptr_t)t.ao.get());
        mix((u64)(uintptr_t)t.emissive.get());

        if(MaterialProperties* p = m->GetProperties())
        {
            auto f = [&](float v)
            { u32 b; memcpy(&b, &v, 4); mix(b); };
            f(p->roughness); f(p->metallic); f(p->reflectance);
            f(p->emissiveColour.x); f(p->emissiveColour.y); f(p->emissiveColour.z); f(p->emissiveColour.w);
            f(p->albedoMapFactor); f(p->metallicMapFactor); f(p->roughnessMapFactor);
            f(p->normalMapFactor); f(p->emissiveMapFactor); f(p->occlusionMapFactor);
            f(p->alphaCutoff); f(p->workflow);
        }
        mix((u64)m->GetFlags());
        return h;
    }

    SceneRenderer::SceneRenderer(uint32_t width, uint32_t height)
    {
        LUMOS_PROFILE_FUNCTION();
        m_CubeMap        = nullptr;
        m_ClearColour    = Vec4(0.2f, 0.2f, 0.2f, 1.0f);
        m_SupportCompute = Renderer::GetCapabilities().SupportCompute;

        Graphics::TextureDesc mainRenderTargetDesc;
        mainRenderTargetDesc.format          = Graphics::RHIFormat::R11G11B10_Float;
        mainRenderTargetDesc.flags           = TextureFlags::Texture_RenderTarget;
        mainRenderTargetDesc.wrap            = TextureWrap::CLAMP_TO_EDGE;
        mainRenderTargetDesc.minFilter       = TextureFilter::LINEAR;
        mainRenderTargetDesc.magFilter       = TextureFilter::LINEAR;
        mainRenderTargetDesc.generateMipMaps = false;

        if(m_SupportCompute)
        {
            mainRenderTargetDesc.format = Graphics::RHIFormat::R16G16B16A16_Float;
            mainRenderTargetDesc.flags  = (TextureFlags)(TextureFlags::Texture_RenderTarget | TextureFlags::Texture_Storage);
        }

        mainRenderTargetDesc.samples = m_MainTextureSamples;
        m_MainTexture                = Graphics::Texture2D::Create(mainRenderTargetDesc, width, height);
        mainRenderTargetDesc.samples = 1;
        m_PostProcessTexture1        = Graphics::Texture2D::Create(mainRenderTargetDesc, width, height);
        m_ResolveTexture             = Graphics::Texture2D::Create(mainRenderTargetDesc, 1, 1);
        m_TargetWidth                = width;
        m_TargetHeight               = height;

        // Setup shadow pass data
        m_ShadowData.m_ShadowTex    = nullptr;
        m_ShadowData.m_ShadowMapNum = 4;

        uint32_t shadowMapResolution = 4096;
        switch(Application::Get().GetQualitySettings().ShadowResolution)
        {
        case ShadowResolutionSetting::Low:
            shadowMapResolution = 1024;
            break;
        case ShadowResolutionSetting::Medium:
            shadowMapResolution = 2048;
            break;
        case ShadowResolutionSetting::High:
            shadowMapResolution = 4096;
            break;
        }

        m_ShadowData.m_ShadowMapSize         = shadowMapResolution;
        m_ShadowData.m_ShadowMapsInvalidated = true;
        m_ShadowData.m_CascadeSplitLambda    = 0.92f;
        m_ShadowData.m_Shader                = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("Shadow")).As<Graphics::Shader>();
        m_ShadowData.m_ShaderAlpha           = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("ShadowAlpha")).As<Graphics::Shader>();
        m_ShadowData.m_ShaderAnim            = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("ShadowAnim")).As<Graphics::Shader>();
        m_ShadowData.m_ShaderAnimAlpha       = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("ShadowAnimAlpha")).As<Graphics::Shader>();
        m_ShadowData.m_ShadowTex             = TextureDepthArray::Create(1, 1, m_ShadowData.m_ShadowMapNum, Renderer::GetRenderer()->GetDepthFormat());
        m_ShadowData.m_LightSize             = 0.6f;
        m_ShadowData.m_MaxShadowDistance     = 500.0f;
        m_ShadowData.m_ShadowFade            = 40.0f;
        m_ShadowData.m_CascadeFade           = 3.0f;
        // Measured in shadow texels, not normalised depth - the shader converts it
        // per cascade, so one value stays correct as texel size grows with distance.
        m_ShadowData.m_InitialBias           = 1.0f;

        Graphics::DescriptorDesc descriptorDesc {};
        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader      = m_ShadowData.m_Shader.get();
        m_ShadowData.m_DescriptorSet.Resize(2);
        m_ShadowData.m_DescriptorSet[0] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
        descriptorDesc.shader           = m_ShadowData.m_ShaderAlpha.get();
        m_ShadowData.m_DescriptorSet[1] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));

        m_ShadowData.m_CascadeCommandQueue[0].Reserve(1000);
        m_ShadowData.m_CascadeCommandQueue[1].Reserve(1000);
        m_ShadowData.m_CascadeCommandQueue[2].Reserve(1000);
        m_ShadowData.m_CascadeCommandQueue[3].Reserve(1000);

        // Setup forward pass data
        m_ForwardData.m_DepthTest    = true;
#ifdef LUMOS_PLATFORM_MACOS
        m_ForwardData.m_Shader       = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("ForwardPBR_Low")).As<Graphics::Shader>();
#else
        m_ForwardData.m_Shader       = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("ForwardPBR")).As<Graphics::Shader>();
#endif
        m_ForwardData.m_AnimShader   = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("ForwardPBRAnim")).As<Graphics::Shader>();
        m_ForwardData.m_DepthTexture = TextureDepth::Create(width, height, Renderer::GetRenderer()->GetDepthFormat(), m_MainTextureSamples);
        m_ForwardData.m_CommandQueue.Reserve(1000);

        const uint32_t instanceFrameCount = Renderer::GetMainSwapChain()->GetSwapChainBufferCount();

        m_ForwardData.m_InstancedShader = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("ForwardPBRInstanced")).As<Graphics::Shader>();

        if(m_ForwardData.m_InstancedShader)
        {
            for(uint32_t f = 0; f < instanceFrameCount; f++)
            {
                StorageBuffer* ssbo = StorageBuffer::Create(MAX_INSTANCE_COUNT * sizeof(ForwardInstanceData));
                Graphics::DescriptorDesc instanceDesc {};
                instanceDesc.layoutIndex = 3;
                instanceDesc.shader      = m_ForwardData.m_InstancedShader.get();
                SharedPtr<Graphics::DescriptorSet> set(Graphics::DescriptorSet::Create(instanceDesc));
                set->SetStorageBuffer(0, ssbo);
                set->Update();

                m_ForwardData.m_InstanceTransformSSBO.PushBack(ssbo);
                m_ForwardData.m_InstanceDescriptorSet.PushBack(set);
            }
        }

        m_ShadowData.m_ShaderInstanced      = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("ShadowInstanced")).As<Graphics::Shader>();
        m_ShadowData.m_ShaderInstancedAlpha = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("ShadowInstancedAlpha")).As<Graphics::Shader>();

        if(m_ShadowData.m_ShaderInstanced)
        {
            for(uint32_t f = 0; f < instanceFrameCount; f++)
            {
                StorageBuffer* ssbo = StorageBuffer::Create(MAX_INSTANCE_COUNT * sizeof(Mat4));
                Graphics::DescriptorDesc instanceDesc {};
                instanceDesc.layoutIndex = 3;
                instanceDesc.shader      = m_ShadowData.m_ShaderInstanced.get();
                SharedPtr<Graphics::DescriptorSet> set(Graphics::DescriptorSet::Create(instanceDesc));
                set->SetStorageBuffer(0, ssbo);
                set->Update();

                m_ShadowData.m_InstanceTransformSSBO.PushBack(ssbo);
                m_ShadowData.m_InstanceDescriptorSet.PushBack(set);
            }
        }

        const size_t minUboAlignment = size_t(Graphics::Renderer::GetCapabilities().UniformBufferOffsetAlignment);

        m_ForwardData.m_DynamicAlignment = sizeof(Mat4);
        if(minUboAlignment > 0)
        {
            m_ForwardData.m_DynamicAlignment = (m_ForwardData.m_DynamicAlignment + minUboAlignment - 1) & ~(minUboAlignment - 1);
        }

        m_ForwardData.m_TransformData = static_cast<Mat4*>(Memory::AlignedAlloc(static_cast<uint32_t>(MAX_OBJECTS * m_ForwardData.m_DynamicAlignment), m_ForwardData.m_DynamicAlignment));

        const int SSAO_NOISE_DIM = 4;
        TDArray<Vec4> noiseData(SSAO_NOISE_DIM * SSAO_NOISE_DIM);

        for(Vec4& noiseSample : noiseData)
        {
            // Random rotations around z-axis
            noiseSample = Vec4(Random32::Rand(-1.0f, 1.0f), Random32::Rand(-1.0f, 1.0f), 0.0f, 0.0f);
        }

        Graphics::TextureDesc noiseTextureDesc = {};
        noiseTextureDesc.format                = Graphics::RHIFormat::R32G32B32A32_Float;
        noiseTextureDesc.wrap                  = TextureWrap::REPEAT;
        noiseTextureDesc.minFilter             = TextureFilter::NEAREST;
        noiseTextureDesc.magFilter             = TextureFilter::NEAREST;
        noiseTextureDesc.generateMipMaps       = false;
        noiseTextureDesc.anisotropicFiltering  = false;
        noiseTextureDesc.flags                 = 0;
        m_NoiseTexture                         = Graphics::Texture2D::CreateFromSource(SSAO_NOISE_DIM, SSAO_NOISE_DIM, (void*)noiseData.Data(), noiseTextureDesc);

        noiseTextureDesc.flags     = TextureFlags::Texture_RenderTarget;
        noiseTextureDesc.wrap      = TextureWrap::CLAMP_TO_EDGE;
        noiseTextureDesc.minFilter = TextureFilter::LINEAR;
        noiseTextureDesc.magFilter = TextureFilter::LINEAR;
        noiseTextureDesc.samples   = m_MainTextureSamples;
        m_NormalTexture            = Graphics::Texture2D::Create(noiseTextureDesc, 1, 1);

        // Always single-sample; receives the resolved G-buffer under MSAA.
        noiseTextureDesc.samples   = 1;
        m_NormalResolveTexture     = Graphics::Texture2D::Create(noiseTextureDesc, 1, 1);
        noiseTextureDesc.samples   = m_MainTextureSamples;

        m_SSAOTexture  = Graphics::Texture2D::Create(noiseTextureDesc, 1, 1);
        m_SSAOTexture1 = Graphics::Texture2D::Create(noiseTextureDesc, 1, 1);

        switch(Graphics::GraphicsContext::GetRenderAPI())
        {
            // TODO: Check

#ifdef LUMOS_RENDER_API_VULKAN
        case Graphics::RenderAPI::VULKAN:
            m_ForwardData.m_BiasMatrix = Mat4(
                0.5, 0.0, 0.0, 0.0,
                0.0, 0.5, 0.0, 0.0,
                0.0, 0.0, 1.0, 0.0,
                0.5, 0.5, 0.0, 1.0);
#endif

#ifdef LUMOS_RENDER_API_DIRECT3D
        case Graphics::RenderAPI::DIRECT3D:
            m_ForwardData.m_BiasMatrix = Mat4(0.5f, 0.0f, 0.0f, 0.5f, 0.0f, 0.5f, 0.0f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            break;
#endif
        default:
            break;
        }

        TextureDesc param;
        param.minFilter         = TextureFilter::LINEAR;
        param.magFilter         = TextureFilter::LINEAR;
        param.format            = RHIFormat::R16G16_Float;
        param.srgb              = false;
        param.wrap              = TextureWrap::CLAMP_TO_EDGE;
        param.flags             = TextureFlags::Texture_RenderTarget;
        param.generateMipMaps   = false;
        const auto& brdfData = Embedded::GetBRDFData();
        m_ForwardData.m_BRDFLUT = UniquePtr<Texture2D>(Texture2D::Create(param, brdfData.width, brdfData.height));

        m_GenerateBRDFLUT = true;

        auto descriptorSetScene    = m_ForwardData.m_Shader->GetDescriptorInfo(2);
        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader      = m_ForwardData.m_Shader.get();
        m_ForwardData.m_DescriptorSet.Resize(4);
        m_ForwardData.m_DescriptorSet[0] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
        descriptorDesc.layoutIndex       = 2;
        m_ForwardData.m_DescriptorSet[2] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
        descriptorDesc.layoutIndex       = 3;
        descriptorDesc.shader            = m_ForwardData.m_AnimShader;
        m_ForwardData.m_DescriptorSet[3] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
        Mat4 boneTransforms[256]; // MAX_BONES in Buffers.glslh
        for(int i = 0; i < 256; i++)
        {
            boneTransforms[i] = Mat4(1.0f);
        }
        m_ForwardData.m_DescriptorSet[3]->SetUniformBufferData(0, boneTransforms);
        m_ForwardData.m_DescriptorSet[3]->Update();

        m_ForwardData.m_DefaultMaterial  = new Material(m_ForwardData.m_Shader);
        uint32_t blackCubeTextureData[6] = { 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000 };
        m_DefaultTextureCube             = Graphics::TextureCube::Create(1, blackCubeTextureData);

        Graphics::MaterialProperties properties;
        properties.albedoColour       = Vec4(1.0f);
        properties.roughness          = 0.5f;
        properties.metallic           = 0.5f;
        properties.albedoMapFactor    = 0.0f;
        properties.roughnessMapFactor = 0.0f;
        properties.normalMapFactor    = 0.0f;
        properties.metallicMapFactor  = 0.0f;

        m_ForwardData.m_DefaultMaterial->SetMaterialProperites(properties);
        // m_ForwardData.m_DefaultMaterial->CreateDescriptorSet(1);

        // Forward+ light culling setup
        if(m_SupportCompute)
        {
            m_ForwardPlusEnabled = true;
            m_TileSize   = FORWARD_PLUS_TILE_SIZE;
            m_TileCountX = (width + m_TileSize - 1) / m_TileSize;
            m_TileCountY = (height + m_TileSize - 1) / m_TileSize;

            m_LightCullingShader = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("LightCulling")).As<Graphics::Shader>();

            uint32_t tileCount = m_TileCountX * m_TileCountY;
            m_LightSSBO           = Graphics::StorageBuffer::Create(MAX_LIGHTS * sizeof(Graphics::GPULight));
            m_LightGridSSBO       = Graphics::StorageBuffer::Create(tileCount * sizeof(uint32_t) * 2);
            // one tile's worth until light culling first runs (full size is ~16MB at 4K)
            m_LightIndexSSBO      = Graphics::StorageBuffer::Create(MAX_LIGHTS_PER_TILE * sizeof(uint32_t));
            m_GlobalIndexCountSSBO = Graphics::StorageBuffer::Create(sizeof(uint32_t));

            if(m_LightCullingShader && m_LightCullingShader->IsCompiled())
            {
                descriptorDesc.layoutIndex = 0;
                descriptorDesc.shader      = m_LightCullingShader.get();
                m_LightCullingDescriptorSet = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
            }
            else
            {
                LWARN("LightCulling shader not available, falling back to brute-force lighting");
                m_ForwardPlusEnabled = false;
            }

            // Use ForwardPBRPlus shader for Forward+ path
            if(m_ForwardPlusEnabled)
            {
                auto forwardPlusShader = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("ForwardPBRPlus")).As<Graphics::Shader>();
                if(forwardPlusShader && forwardPlusShader->IsCompiled())
                {
                    m_ForwardData.m_Shader = forwardPlusShader;
                    // Recreate descriptor set 2 from the new shader layout (includes SSBO bindings)
                    descriptorDesc.layoutIndex = 2;
                    descriptorDesc.shader      = m_ForwardData.m_Shader.get();
                    m_ForwardData.m_DescriptorSet[2] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
                }
                else
                {
                    LWARN("ForwardPBRPlus shader not available, falling back to brute-force lighting");
                    m_ForwardPlusEnabled = false;
                }
            }
        }

        // Set up skybox pass data
        m_ScreenQuad               = Graphics::CreateQuad();
        m_SkyboxShader             = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("Skybox")).As<Graphics::Shader>();
        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader      = m_SkyboxShader.get();
        m_SkyboxDescriptorSet      = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));

        // Procedural nebula backdrop (drawn before the skybox/point clouds).
        m_CosmoShader              = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("Cosmo")).As<Graphics::Shader>();
        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader      = m_CosmoShader.get();
        m_CosmoDescriptorSet       = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
        m_CosmoBakeShader          = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("CosmoBake")).As<Graphics::Shader>();
        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader      = m_CosmoBakeShader.get();
        m_CosmoBakeDescriptorSet   = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));

        // Set up final pass data
        m_FinalPassShader          = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("FinalPass")).As<Graphics::Shader>();
        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader      = m_FinalPassShader.get();
        m_FinalPassDescriptorSet   = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));

        m_FinalPassAlphaShader        = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("FinalPassAlpha")).As<Graphics::Shader>();
        if(m_FinalPassAlphaShader)
        {
            descriptorDesc.shader         = m_FinalPassAlphaShader.get();
            m_FinalPassAlphaDescriptorSet = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
        }

        // PostProcesses

        m_ToneMappingPassShader        = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("ToneMapping")).As<Graphics::Shader>();
        descriptorDesc.layoutIndex     = 0;
        descriptorDesc.shader          = m_ToneMappingPassShader.get();
        m_ToneMappingPassDescriptorSet = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));

        m_DepthOfFieldShader            = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("DepthOfField")).As<Graphics::Shader>();
        descriptorDesc.layoutIndex      = 0;
        descriptorDesc.shader           = m_DepthOfFieldShader.get();
        m_DepthOfFieldPassDescriptorSet = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));

        m_SharpenShader            = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("Sharpen")).As<Graphics::Shader>();
        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader      = m_SharpenShader.get();
        m_SharpenPassDescriptorSet = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));

        m_BloomPassShader          = Application::Get().GetAssetManager()->GetAssetData(m_SupportCompute ? Str8Lit("BloomComp") : Str8Lit("Bloom")).As<Graphics::Shader>();
        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader      = m_BloomPassShader.get();

        if(m_BloomPassShader->IsCompiled())
        {
            for(int i = 0; i < 6; i++)
                m_BloomDescriptorSets.PushBack(SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc)));
        }

        {
            float seedAvg = 0.18f;
            m_AverageLuminanceSSBO = Graphics::StorageBuffer::Create(sizeof(float), &seedAvg);
        }

        if(m_SupportCompute)
        {
            m_LuminanceHistogramShader = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("LuminanceHistogram")).As<Graphics::Shader>();
            m_LuminanceAverageShader   = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("LuminanceAverage")).As<Graphics::Shader>();

            if(m_LuminanceHistogramShader && m_LuminanceHistogramShader->IsCompiled() && m_LuminanceAverageShader && m_LuminanceAverageShader->IsCompiled())
            {
                // Starts at zero: the first average is taken over whatever is in here.
                uint32_t emptyHistogram[256] = {};
                m_LuminanceHistogramSSBO = Graphics::StorageBuffer::Create(256 * sizeof(uint32_t), emptyHistogram);

                descriptorDesc.layoutIndex          = 0;
                descriptorDesc.shader               = m_LuminanceHistogramShader.get();
                m_LuminanceHistogramDescriptorSet   = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));

                descriptorDesc.shader               = m_LuminanceAverageShader.get();
                m_LuminanceAverageDescriptorSet     = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));

                m_AdaptiveExposureReady = true;
            }
            else
            {
                LWARN("Adaptive exposure shaders not available — falling back to manual exposure");
            }
        }

        // Forward shader variants: Forward+ when the compute path is on, plain loop otherwise
        {
            auto& assets = *Application::Get().GetAssetManager();
            m_ForwardShaders[0] = m_ForwardData.m_Shader; // Plus, or the platform default when Forward+ is off
            if(m_ForwardPlusEnabled)
                m_ForwardShaders[1] = assets.GetAssetData(Str8Lit("ForwardPBRPlusLow")).As<Graphics::Shader>();
            else
            {
                m_ForwardShaders[0] = assets.GetAssetData(Str8Lit("ForwardPBR")).As<Graphics::Shader>();
                m_ForwardShaders[1] = assets.GetAssetData(Str8Lit("ForwardPBR_Low")).As<Graphics::Shader>();
            }
            m_InstancedShaders[0] = m_ForwardData.m_InstancedShader;
            m_InstancedShaders[1] = assets.GetAssetData(Str8Lit("ForwardPBRInstancedLow")).As<Graphics::Shader>();
        }

        InitOcclusion();

        m_VignetteShader = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("Vignette")).As<Graphics::Shader>();
        if(m_VignetteShader && m_VignetteShader->IsCompiled())
        {
            descriptorDesc.layoutIndex   = 0;
            descriptorDesc.shader        = m_VignetteShader.get();
            m_VignettePassDescriptorSet  = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
        }

        m_MotionBlurShader = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("MotionBlur")).As<Graphics::Shader>();
        if(m_MotionBlurShader && m_MotionBlurShader->IsCompiled())
        {
            descriptorDesc.layoutIndex      = 0;
            descriptorDesc.shader           = m_MotionBlurShader.get();
            m_MotionBlurPassDescriptorSet   = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
        }

        m_SSRShader = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("SSR")).As<Graphics::Shader>();
        if(m_SSRShader && m_SSRShader->IsCompiled())
        {
            descriptorDesc.layoutIndex = 0;
            descriptorDesc.shader      = m_SSRShader.get();
            m_SSRPassDescriptorSet     = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
        }

        // TAA, contact shadows, volumetric fog: targets start at 1x1 and grow on first use
        {
            auto assets   = Application::Get().GetAssetManager();
            auto makeSet  = [&](const SharedPtr<Graphics::Shader>& shader)
            {
                if(!shader || !shader->IsCompiled())
                    return SharedPtr<Graphics::DescriptorSet>();
                descriptorDesc.layoutIndex = 0;
                descriptorDesc.shader      = shader.get();
                return SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
            };
            m_TAAShader                       = assets->GetAssetData(Str8Lit("TAA")).As<Graphics::Shader>();
            m_TAADescriptorSet                = makeSet(m_TAAShader);
            m_ContactShadowShader             = assets->GetAssetData(Str8Lit("ContactShadows")).As<Graphics::Shader>();
            m_ContactShadowDescriptorSet      = makeSet(m_ContactShadowShader);
            m_VolumetricFogShader             = assets->GetAssetData(Str8Lit("VolumetricFog")).As<Graphics::Shader>();
            m_VolumetricFogDescriptorSet      = makeSet(m_VolumetricFogShader);
            m_VolumetricFogApplyShader        = assets->GetAssetData(Str8Lit("VolumetricFogApply")).As<Graphics::Shader>();
            m_VolumetricFogApplyDescriptorSet = makeSet(m_VolumetricFogApplyShader);

            Graphics::TextureDesc targetDesc;
            targetDesc.format          = Graphics::RHIFormat::R16G16B16A16_Float;
            targetDesc.flags           = TextureFlags::Texture_RenderTarget;
            targetDesc.wrap            = TextureWrap::CLAMP_TO_EDGE;
            targetDesc.minFilter       = TextureFilter::LINEAR;
            targetDesc.magFilter       = TextureFilter::LINEAR;
            targetDesc.generateMipMaps = false;
            m_TAAHistory[0]            = Graphics::Texture2D::Create(targetDesc, 1, 1);
            m_TAAHistory[1]            = Graphics::Texture2D::Create(targetDesc, 1, 1);
            m_FogTexture               = Graphics::Texture2D::Create(targetDesc, 1, 1);
            targetDesc.format          = Graphics::RHIFormat::R32_Float;
            targetDesc.minFilter       = TextureFilter::NEAREST;
            targetDesc.magFilter       = TextureFilter::NEAREST;
            m_FogDepthTexture          = Graphics::Texture2D::Create(targetDesc, 1, 1);
            targetDesc.format          = Graphics::RHIFormat::R8_Unorm;
            targetDesc.minFilter       = TextureFilter::LINEAR;
            targetDesc.magFilter       = TextureFilter::LINEAR;
            m_ContactShadowTexture     = Graphics::Texture2D::Create(targetDesc, 1, 1);
        }

        m_GI = new ProbeGI();

        mainRenderTargetDesc.flags = TextureFlags::Texture_RenderTarget | TextureFlags::Texture_CreateMips | TextureFlags::Texture_MipViews;
        if(m_SupportCompute)
        {
            mainRenderTargetDesc.format = Graphics::RHIFormat::R16G16B16A16_Float;
            mainRenderTargetDesc.flags  = (TextureFlags)(TextureFlags::Texture_RenderTarget | TextureFlags::Texture_CreateMips | TextureFlags::Texture_MipViews | TextureFlags::Texture_Storage);
        }
        m_BloomTexture             = Texture2D::Create(mainRenderTargetDesc, 1, 1);
        m_BloomTexture1            = Texture2D::Create(mainRenderTargetDesc, 1, 1);
        m_BloomTexture2            = Texture2D::Create(mainRenderTargetDesc, 1, 1);
        mainRenderTargetDesc.flags  = TextureFlags::Texture_RenderTarget;
        mainRenderTargetDesc.format = Graphics::RHIFormat::R11G11B10_Float;

        m_FXAAShader               = Application::Get().GetAssetManager()->GetAssetData(m_SupportCompute ? Str8Lit("FXAAComp") : Str8Lit("FXAA")).As<Graphics::Shader>();
        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader      = m_FXAAShader.get();
        m_FXAAPassDescriptorSet    = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));

        // SMAA 1x
        {
            m_SMAAEdgesShader   = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("SMAAEdges")).As<Graphics::Shader>();
            m_SMAAWeightsShader = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("SMAAWeights")).As<Graphics::Shader>();
            m_SMAABlendShader   = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("SMAABlend")).As<Graphics::Shader>();

            descriptorDesc.layoutIndex = 0;
            descriptorDesc.shader      = m_SMAAEdgesShader.get();
            m_SMAAEdgesDescriptorSet   = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
            descriptorDesc.shader      = m_SMAAWeightsShader.get();
            m_SMAAWeightsDescriptorSet = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
            descriptorDesc.shader      = m_SMAABlendShader.get();
            m_SMAABlendDescriptorSet   = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));

            Graphics::TextureDesc smaaDesc;
            smaaDesc.format          = Graphics::RHIFormat::R8G8B8A8_Unorm;
            smaaDesc.minFilter       = Graphics::TextureFilter::NEAREST;
            smaaDesc.magFilter       = Graphics::TextureFilter::NEAREST;
            smaaDesc.wrap            = Graphics::TextureWrap::CLAMP;
            smaaDesc.flags           = TextureFlags::Texture_RenderTarget;
            smaaDesc.generateMipMaps = false;
            m_SMAAEdgesTexture       = Graphics::Texture2D::Create(smaaDesc, 1, 1);
            m_SMAAWeightsTexture     = Graphics::Texture2D::Create(smaaDesc, 1, 1);
        }

        m_DebandingShader            = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("Debanding")).As<Graphics::Shader>();
        descriptorDesc.layoutIndex   = 0;
        descriptorDesc.shader        = m_DebandingShader.get();
        m_DebandingPassDescriptorSet = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));

        m_ChromaticAberationShader            = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("ChromaticAberation")).As<Graphics::Shader>();
        descriptorDesc.layoutIndex            = 0;
        descriptorDesc.shader                 = m_ChromaticAberationShader.get();
        m_ChromaticAberationPassDescriptorSet = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));

        m_DepthPrePassShader        = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("DepthPrePass")).As<Graphics::Shader>();
        descriptorDesc.layoutIndex  = 0;
        descriptorDesc.shader       = m_DepthPrePassShader.get();
        m_DepthPrePassDescriptorSet = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));

        m_DepthPrePassAlphaShader        = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("DepthPrePassAlpha")).As<Graphics::Shader>();
        descriptorDesc.shader            = m_DepthPrePassAlphaShader.get();
        m_DepthPrePassAlphaDescriptorSet = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));

        m_DepthPrePassAlphaAnimShader = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("DepthPrePassAlphaAnim")).As<Graphics::Shader>();

        m_DepthPrePassInstancedShader = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("DepthPrePassInstanced")).As<Graphics::Shader>();
        m_DepthPrePassAnimShader       = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("DepthPrePassAnim")).As<Graphics::Shader>();
        m_FilmicGrainShader            = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("FilmicGrain")).As<Graphics::Shader>();
        descriptorDesc.layoutIndex     = 0;
        descriptorDesc.shader          = m_FilmicGrainShader.get();
        m_FilmicGrainPassDescriptorSet = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));

        m_SSAOShader               = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("SSAO")).As<Graphics::Shader>();
        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader      = m_SSAOShader.get();

        if(m_SSAOShader->IsCompiled())
            m_SSAOPassDescriptorSet = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));

        m_SSAOBlurShader           = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("SSAOBlur")).As<Graphics::Shader>();
        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader      = m_SSAOBlurShader.get();
        if(m_SSAOBlurShader->IsCompiled())
        {
            m_SSAOBlurPassDescriptorSet  = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
            m_SSAOBlurPassDescriptorSet2 = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
        }

        // m_OutlineShader = Application::Get().GetAssetManager()->GetAssetData("Outline");
        //         descriptorDesc.layoutIndex = 0;
        //         descriptorDesc.shader      = m_OutlineShader.get();
        //         m_OutlinePassDescriptorSet    = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));

        // Setup 2D pass data
        m_Renderer2DData.m_IndexCount           = 0;
        m_Renderer2DData.m_Buffer               = nullptr;
        m_Renderer2DData.m_RenderToDepthTexture = true;
        m_Renderer2DData.m_TriangleIndicies     = false;
        m_Renderer2DData.m_Limits.SetMaxQuads(10000);
        m_Renderer2DData.m_Limits.MaxTextures = 16; // Renderer::GetCapabilities().MaxTextureUnits;

        m_Renderer2DData.m_Shader = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("Batch2D")).As<Graphics::Shader>();

        m_Renderer2DData.m_TransformationStack.EmplaceBack(Mat4(1.0f));
        m_Renderer2DData.m_TransformationBack = &m_Renderer2DData.m_TransformationStack.Back();

        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader      = m_Renderer2DData.m_Shader.get();
        m_Renderer2DData.m_DescriptorSet.Resize(m_Renderer2DData.m_Limits.MaxBatchDrawCalls);
        m_Renderer2DData.m_PreviousFrameTextureCount.Resize(m_Renderer2DData.m_Limits.MaxBatchDrawCalls);

        for(uint32_t i = 0; i < m_Renderer2DData.m_Limits.MaxBatchDrawCalls; i++)
        {
            m_Renderer2DData.m_PreviousFrameTextureCount[i] = 0;
            m_Renderer2DData.m_DescriptorSet[i].Resize(2);
            // if(i == 0)
            {
                descriptorDesc.layoutIndex             = 0;
                m_Renderer2DData.m_DescriptorSet[0][0] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
            }
            descriptorDesc.layoutIndex             = 1;
            m_Renderer2DData.m_DescriptorSet[i][1] = nullptr; // SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
        }

        m_Renderer2DData.m_VertexBuffers.Resize(Renderer::GetMainSwapChain()->GetSwapChainBufferCount());

        uint32_t* indices = new uint32_t[m_Renderer2DData.m_Limits.IndiciesSize];

        if(m_Renderer2DData.m_TriangleIndicies)
        {
            for(uint32_t i = 0; i < m_Renderer2DData.m_Limits.IndiciesSize; i++)
            {
                indices[i] = i;
            }
        }
        else
        {
            uint32_t offset = 0;
            for(uint32_t i = 0; i < m_Renderer2DData.m_Limits.IndiciesSize; i += 6)
            {
                indices[i]     = offset + 0;
                indices[i + 1] = offset + 1;
                indices[i + 2] = offset + 2;

                indices[i + 3] = offset + 2;
                indices[i + 4] = offset + 3;
                indices[i + 5] = offset + 0;

                offset += 4;
            }
        }
        m_Renderer2DData.m_IndexBuffer = IndexBuffer::Create(indices, m_Renderer2DData.m_Limits.IndiciesSize);
        m_2DBufferBase.Resize(Renderer::GetMainSwapChain()->GetSwapChainBufferCount());

        for(int currentFrame = 0; currentFrame < Renderer::GetMainSwapChain()->GetSwapChainBufferCount(); currentFrame++)
        {
            m_Renderer2DData.m_VertexBuffers[currentFrame].EmplaceBack(Graphics::VertexBuffer::Create(BATCH_BUFFER_INITIAL, nullptr, BufferUsage::DYNAMIC));

            // one staging array per frame: each batch is uploaded at flush, before the next one writes
            m_2DBufferBase[currentFrame].EmplaceBack(new VertexData[m_Renderer2DData.m_Limits.MaxQuads * 4]);
        }

        Init2DLitRenderData();

        delete[] indices;

        {
            // Setup Particle pass data
            m_ParticleData.m_IndexCount           = 0;
            m_ParticleData.m_Buffer               = nullptr;
            m_ParticleData.m_RenderToDepthTexture = true;
            m_ParticleData.m_TriangleIndicies     = false;
            m_ParticleData.m_Limits.SetMaxQuads(10000);
            m_ParticleData.m_Limits.MaxTextures = 16; // Renderer::GetCapabilities().MaxTextureUnits;

            m_ParticleData.m_Shader = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("Particle")).As<Graphics::Shader>();

            m_ParticleData.m_TransformationStack.EmplaceBack(Mat4(1.0f));
            m_ParticleData.m_TransformationBack = &m_ParticleData.m_TransformationStack.Back();

            descriptorDesc.layoutIndex = 0;
            descriptorDesc.shader      = m_ParticleData.m_Shader.get();
            m_ParticleData.m_DescriptorSet.Resize(m_ParticleData.m_Limits.MaxBatchDrawCalls);
            m_ParticleData.m_PreviousFrameTextureCount.Resize(m_ParticleData.m_Limits.MaxBatchDrawCalls);

            for(uint32_t i = 0; i < m_ParticleData.m_Limits.MaxBatchDrawCalls; i++)
            {
                m_ParticleData.m_PreviousFrameTextureCount[i] = 0;
                m_ParticleData.m_DescriptorSet[i].Resize(2);
                if(i == 0)
                {
                    descriptorDesc.layoutIndex           = 0;
                    m_ParticleData.m_DescriptorSet[0][0] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
                }
                descriptorDesc.layoutIndex           = 1;
                m_ParticleData.m_DescriptorSet[i][1] = nullptr; // SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
            }

            m_ParticleData.m_VertexBuffers.Resize(Renderer::GetMainSwapChain()->GetSwapChainBufferCount());

            uint32_t* indices = new uint32_t[m_ParticleData.m_Limits.IndiciesSize];

            if(m_ParticleData.m_TriangleIndicies)
            {
                for(uint32_t i = 0; i < m_ParticleData.m_Limits.IndiciesSize; i++)
                {
                    indices[i] = i;
                }
            }
            else
            {
                uint32_t offset = 0;
                for(uint32_t i = 0; i < m_ParticleData.m_Limits.IndiciesSize; i += 6)
                {
                    indices[i]     = offset + 0;
                    indices[i + 1] = offset + 1;
                    indices[i + 2] = offset + 2;

                    indices[i + 3] = offset + 2;
                    indices[i + 4] = offset + 3;
                    indices[i + 5] = offset + 0;

                    offset += 4;
                }
            }
            m_ParticleData.m_IndexBuffer = IndexBuffer::Create(indices, m_ParticleData.m_Limits.IndiciesSize);
            m_ParticleBufferBase.Resize(Renderer::GetMainSwapChain()->GetSwapChainBufferCount());

            for(int currentFrame = 0; currentFrame < Renderer::GetMainSwapChain()->GetSwapChainBufferCount(); currentFrame++)
            {
                auto& vertexBuffer = m_ParticleData.m_VertexBuffers[currentFrame].EmplaceBack(Graphics::VertexBuffer::Create(BATCH_BUFFER_INITIAL, nullptr, BufferUsage::DYNAMIC));

                m_ParticleBufferBase[currentFrame].EmplaceBack(new VertexData[m_ParticleData.m_Limits.MaxQuads * 4]);
            }

            delete[] indices;
        }

        // Setup text pass
        m_TextRendererData.m_IndexCount = 0;
        // m_TextBuffer                              = nullptr;
        m_TextRendererData.m_RenderToDepthTexture = true;
        m_TextRendererData.m_TriangleIndicies     = false;
        m_TextRendererData.m_Limits.SetMaxQuads(10000);
        m_TextRendererData.m_Limits.MaxTextures = 16; // Renderer::GetCapabilities().MaxTextureUnits;

        // sized AFTER the limit: allocated before it, the buffer held the default 1000 quads
        // while the flush guard allowed 10000 - a glyph-heavy UI frame wrote past its end
        for(int i = 0; i < Renderer::GetMainSwapChain()->GetSwapChainBufferCount(); i++)
        {
            TextVertexBufferBase.PushBack(new TextVertexData[m_TextRendererData.m_Limits.MaxQuads * 4]);
        }

        TextVertexBufferPtr = TextVertexBufferBase[0];

        m_TextRendererData.m_Shader = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("Text")).As<Graphics::Shader>();

        m_TextRendererData.m_TransformationStack.EmplaceBack(Mat4(1.0f));
        m_TextRendererData.m_TransformationBack = &m_Renderer2DData.m_TransformationStack.Back();

        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader      = m_Renderer2DData.m_Shader.get();
        m_TextRendererData.m_DescriptorSet.Resize(m_Renderer2DData.m_Limits.MaxBatchDrawCalls);
        m_TextRendererData.m_PreviousFrameTextureCount.Resize(m_Renderer2DData.m_Limits.MaxBatchDrawCalls);

        for(uint32_t i = 0; i < m_TextRendererData.m_Limits.MaxBatchDrawCalls; i++)
        {
            m_TextRendererData.m_PreviousFrameTextureCount[i] = 0;
            m_TextRendererData.m_DescriptorSet[i].Resize(2);
            // if(i == 0)
            {
                descriptorDesc.layoutIndex               = 0;
                m_TextRendererData.m_DescriptorSet[i][0] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
            }
            descriptorDesc.layoutIndex               = 1;
            m_TextRendererData.m_DescriptorSet[i][1] = nullptr; // SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
        }

        m_TextRendererData.m_VertexBuffers.Resize(Renderer::GetMainSwapChain()->GetSwapChainBufferCount());
        indices = new uint32_t[m_TextRendererData.m_Limits.IndiciesSize];

        if(m_TextRendererData.m_TriangleIndicies)
        {
            for(uint32_t i = 0; i < m_TextRendererData.m_Limits.IndiciesSize; i++)
            {
                indices[i] = i;
            }
        }
        else
        {
            uint32_t offset = 0;
            for(uint32_t i = 0; i < m_TextRendererData.m_Limits.IndiciesSize; i += 6)
            {
                indices[i]     = offset + 0;
                indices[i + 1] = offset + 1;
                indices[i + 2] = offset + 2;

                indices[i + 3] = offset + 2;
                indices[i + 4] = offset + 3;
                indices[i + 5] = offset + 0;

                offset += 4;
            }
        }
        m_TextRendererData.m_IndexBuffer = IndexBuffer::Create(indices, m_TextRendererData.m_Limits.IndiciesSize);

        delete[] indices;

        InitDebugRenderData();
    }

    static SceneRenderer* s_GameRenderer = nullptr;

    SceneRenderer* SceneRenderer::GetGameRenderer()
    {
        return s_GameRenderer ? s_GameRenderer : Application::Get().GetSceneRenderer();
    }

    void SceneRenderer::SetGameRenderer(SceneRenderer* renderer)
    {
        s_GameRenderer = renderer;
    }

    SceneRenderer::~SceneRenderer()
    {
        delete m_Probe.Cam;
        delete m_Probe.Trans;
        if(s_GameRenderer == this)
            s_GameRenderer = nullptr;

        if(s_SkyNoiseTexture)
        {
            delete s_SkyNoiseTexture;
            s_SkyNoiseTexture = nullptr;
        }

        Graphics::ClearPointClouds();
        Graphics::ClearLineClouds();
        Graphics::ReleaseImmediateLineClouds();
        Graphics::ClearWorldLabels();
        Graphics::ClearPlanets();
        Graphics::PlanetReleaseBuffer();
        delete m_PlanetQuadVB;
        delete m_PlanetQuadIB;
        delete m_PointCloudQuadVB;
        delete m_PointCloudQuadIB;
        delete m_LineCloudQuadVB;
        delete m_LineCloudQuadIB;

        Memory::AlignedFree(m_ForwardData.m_TransformData);

        delete m_ForwardData.m_DepthTexture;
        delete m_MainTexture;
        delete m_ResolveTexture;
        delete m_PostProcessTexture1;
        delete m_SMAAEdgesTexture;
        delete m_CosmoSky[0];
        delete m_CosmoSky[1];
        delete m_SMAAWeightsTexture;
        delete m_SSAOTexture;
        delete m_SSAOTexture1;
        delete m_NoiseTexture;
        delete m_BloomTexture;
        delete m_BloomTexture1;
        delete m_BloomTexture2;
        delete m_NormalTexture;
        delete m_NormalResolveTexture;
        delete m_TAAHistory[0];
        delete m_TAAHistory[1];
        delete m_ContactShadowTexture;
        delete m_FogTexture;
        delete m_FogDepthTexture;
        delete m_GI;

        delete m_ShadowData.m_ShadowTex;
        for(CascadeSchedule& entry : s_CascadeSchedules)
        {
            if(entry.Owner != this)
                continue;
            for(TextureDepth*& cascade : entry.Cascades)
            {
                delete cascade;
                cascade = nullptr;
            }
            entry = CascadeSchedule();
        }
        delete m_ForwardData.m_DefaultMaterial;
        delete m_DefaultTextureCube;
        delete m_ScreenQuad;

        delete m_LightSSBO;
        delete m_LightGridSSBO;
        delete m_LightIndexSSBO;
        for(StorageBuffer* buffer : m_Occlusion.Buffers)
            delete buffer;
        delete m_GlobalIndexCountSSBO;
        delete m_LuminanceHistogramSSBO;
        delete m_AverageLuminanceSSBO;

        for(StorageBuffer* ssbo : m_ForwardData.m_InstanceTransformSSBO)
            delete ssbo;
        for(StorageBuffer* ssbo : m_ShadowData.m_InstanceTransformSSBO)
            delete ssbo;

        delete m_Renderer2DData.m_IndexBuffer;
        delete m_TextRendererData.m_IndexBuffer;
        delete m_ParticleData.m_IndexBuffer;
        if(m_LitDataInitialised)
            delete m_LitData.m_IndexBuffer;

        if(m_DebugRenderDataInitialised)
        {
            delete m_DebugTextRendererData.m_IndexBuffer;
            delete m_DebugDrawData.m_Renderer2DData.m_IndexBuffer;
            delete m_DebugDrawData.m_LineIndexBuffer;
            delete m_DebugDrawData.m_PointIndexBuffer;
        }

        for(auto data : TextVertexBufferBase)
            delete[] data;
        for(auto data : DebugTextVertexBufferBase)
            delete[] data;

        for(auto& lineBufferBase : m_LineBufferBase)
            for(auto data : lineBufferBase)
                delete[] data;
        for(auto& pointBufferBase : m_PointBufferBase)
            for(auto data : pointBufferBase)
                delete[] data;
        for(auto& quadBufferBase : m_QuadBufferBase)
            for(auto data : quadBufferBase)
                delete[] data;

        for(int j = 0; j < Renderer::GetMainSwapChain()->GetSwapChainBufferCount(); j++)
        {
            for(size_t i = 0; i < m_Renderer2DData.m_VertexBuffers[j].Size(); i++)
            {
                delete m_Renderer2DData.m_VertexBuffers[j][i];
            }

            for(uint32_t i = 0; i < m_TextRendererData.m_VertexBuffers[j].Size(); i++)
            {
                delete m_TextRendererData.m_VertexBuffers[j][i];
            }

            for(size_t i = 0; i < m_ParticleData.m_VertexBuffers[j].Size(); i++)
            {
                delete m_ParticleData.m_VertexBuffers[j][i];
            }

            if(m_DebugRenderDataInitialised)
                for(uint32_t i = 0; i < m_DebugTextRendererData.m_VertexBuffers[j].Size(); i++)
                {
                    delete m_DebugTextRendererData.m_VertexBuffers[j][i];
                }

            if(m_DebugRenderDataInitialised)
                for(size_t i = 0; i < m_DebugDrawData.m_Renderer2DData.m_VertexBuffers[j].Size(); i++)
                {
                    delete m_DebugDrawData.m_Renderer2DData.m_VertexBuffers[j][i];
                }

            for(auto data : m_2DBufferBase[j])
                delete[] data;

            for(auto data : m_ParticleBufferBase[j])
                delete[] data;

            if(m_LitDataInitialised)
            {
                for(size_t i = 0; i < m_LitData.m_VertexBuffers[j].Size(); i++)
                    delete m_LitData.m_VertexBuffers[j][i];
                for(auto data : m_LitBufferBase[j])
                    delete[] data;
            }
        }

        if(m_DebugRenderDataInitialised)
        {
            if(!m_DebugDrawData.m_PointVertexBuffers.Empty())
                for(int i = 0; i < m_DebugDrawData.m_PointVertexBuffers[0].Size(); i++)
                {
                    delete m_DebugDrawData.m_PointVertexBuffers[0][i];
                }

            if(!m_DebugDrawData.m_LineVertexBuffers.Empty())
                for(int i = 0; i < m_DebugDrawData.m_LineVertexBuffers[0].Size(); i++)
                {
                    delete m_DebugDrawData.m_LineVertexBuffers[0][i];
                }
        }
    }

    void SceneRenderer::OnResize(uint32_t width, uint32_t height)
    {
        LUMOS_PROFILE_FUNCTION();

        width -= (width % 2 != 0) ? 1 : 0;
        height -= (height % 2 != 0) ? 1 : 0;

        const QualitySettings& qs = Application::Get().GetQualitySettings();
        width                     = uint32_t(qs.RendererScale * m_ViewScale * float(width));
        height                    = uint32_t(qs.RendererScale * m_ViewScale * float(height));

        // a window squeezed to nothing (or mid-minimise): keep the targets it had, Vulkan has no 0-wide image
        if(width < 2 || height < 2)
            return;

        m_TargetWidth  = width;
        m_TargetHeight = height;
        m_ForwardData.m_DepthTexture->Resize(width, height);
        m_MainTexture->Resize(width, height);
        m_PostProcessTexture1->Resize(width, height);
        // the rest only once something has used them
        if(m_ResolveAllocated)
            m_ResolveTexture->Resize(width, height);
        if(m_SMAAAllocated)
        {
            m_SMAAEdgesTexture->Resize(width, height);
            m_SMAAWeightsTexture->Resize(width, height);
        }
        if(m_BloomAllocated)
        {
            m_BloomTexture->Resize(width / 2, height / 2);
            m_BloomTexture1->Resize(width / 2, height / 2);
            m_BloomTexture2->Resize(width / 2, height / 2);
        }
        if(m_GBufferAllocated)
        {
            m_SSAOTexture->Resize(width / 2, height / 2);
            m_SSAOTexture1->Resize(width / 2, height / 2);
            m_NormalTexture->Resize(width, height);
            m_NormalResolveTexture->Resize(width, height);
        }
        if(m_TAAAllocated)
        {
            m_TAAHistory[0]->Resize(width, height);
            m_TAAHistory[1]->Resize(width, height);
        }
        m_TAAHistoryValid = false;
        if(m_ContactShadowsAllocated)
            m_ContactShadowTexture->Resize(width, height);
        m_ContactShadowsValid = false;
        if(m_FogAllocated)
        {
            m_FogTexture->Resize(Maths::Max(1u, width / 2), Maths::Max(1u, height / 2));
            m_FogDepthTexture->Resize(Maths::Max(1u, width / 2), Maths::Max(1u, height / 2));
        }

        // Resize Forward+ buffers
        if(m_ForwardPlusEnabled)
        {
            m_TileCountX = (width + m_TileSize - 1) / m_TileSize;
            m_TileCountY = (height + m_TileSize - 1) / m_TileSize;
            uint32_t tileCount = m_TileCountX * m_TileCountY;
            m_LightGridSSBO->Resize(tileCount * sizeof(uint32_t) * 2, nullptr);
            m_StaticGridLights = ~0u;
            if(m_LightIndexAllocated)
                m_LightIndexSSBO->Resize(tileCount * MAX_LIGHTS_PER_TILE * sizeof(uint32_t), nullptr);
        }
    }

    void SceneRenderer::EnableDebugRenderer(bool enable)
    {
        m_DebugRenderEnabled = enable;

        if(m_DebugRenderEnabled)
        {
            DebugRenderer::Init();
            InitDebugRenderData();
        }
        else
            DebugRenderer::Release();
    }

    void SceneRenderer::BeginScene(Scene* scene)
    {
        LUMOS_PROFILE_FUNCTION();
        auto& registry             = scene->GetRegistry();
        m_CurrentScene             = scene;
        m_Stats.FramesPerSecond    = 0;
        m_Stats.NumDrawCalls        = 0;
        m_Stats.NumRenderedObjects  = 0;
        m_Stats.NumShadowObjects    = 0;
        m_Stats.NumInstanceBatches  = 0;
        m_Stats.NumInstancedObjects = 0;
        m_Stats.NumOcclusionCulled  = 0;
        m_Stats.NumSmallCulled      = 0;
        m_Stats.NumLODMeshes        = 0;
        m_Stats.UpdatesPerSecond   = 0;

        ApplyForwardQuality();

        m_Renderer2DData.m_BatchDrawCallIndex        = 0;
        m_TextRendererData.m_BatchDrawCallIndex      = 0;
        m_DebugTextRendererData.m_BatchDrawCallIndex = 0;
        m_ParticleData.m_BatchDrawCallIndex          = 0;
        m_LitData.m_BatchDrawCallIndex               = 0;

        FinishProbe();
        if(m_Probe.Face >= 0 && m_Probe.Cam && m_MainTexture)
        {
            // looking down the axis of the face being captured (GetCubeMapTexCoord2's order); 100 degrees covers its corners
            static const Vec2 faceAngles[6] = { { 0.0f, -90.0f }, { 0.0f, 90.0f }, { 90.0f, 0.0f }, { -90.0f, 0.0f }, { 0.0f, 180.0f }, { 0.0f, 0.0f } };
            m_Probe.Cam->SetAspectRatio((float)m_MainTexture->GetWidth() / (float)Maths::Max(1u, m_MainTexture->GetHeight()));
            m_Probe.Trans->SetLocalPosition(m_Probe.Position);
            m_Probe.Trans->SetLocalOrientation(Quat::EulerAnglesToQuaternion(faceAngles[m_Probe.Face].x, faceAngles[m_Probe.Face].y, 0.0f));
            m_Probe.Trans->SetWorldMatrix(Mat4(1.0f));
            m_Camera          = m_Probe.Cam;
            m_CameraTransform = m_Probe.Trans;
            m_Probe.ProjView  = m_Camera->GetProjectionMatrix() * m_CameraTransform->GetWorldMatrix().Inverse();
        }
        else if(m_OverrideCamera)
        {
            m_Camera          = m_OverrideCamera;
            m_CameraTransform = m_OverrideCameraTransform;
        }
        else
        {
            auto cameraView = registry.view<Camera>();
            if(!cameraView.empty())
            {
                m_Camera          = &cameraView.get<Camera>(cameraView.front());
                m_CameraTransform = registry.try_get<Maths::Transform>(cameraView.front());
            }
        }

        if(!m_Camera || !m_CameraTransform)
            return;

        // the scene's Exposure is compensation on top of the camera's (1 = as the camera meters)
        m_Exposure     = m_Camera->GetExposure() * scene->GetSettings().RenderSettings.m_Exposure;
        m_ToneMapIndex = scene->GetSettings().RenderSettings.m_ToneMapIndex;

        if(m_MainTextureSamples != scene->GetSettings().RenderSettings.MSAASamples)
        {
            m_MainTextureSamples = scene->GetSettings().RenderSettings.MSAASamples;

            uint32_t width  = m_MainTexture->GetWidth();
            uint32_t height = m_MainTexture->GetHeight();

            delete m_MainTexture;
            delete m_ForwardData.m_DepthTexture;
            delete m_NormalTexture;
            delete m_NormalResolveTexture;
            delete m_SSAOTexture;
            delete m_SSAOTexture1;

            Graphics::TextureDesc mainRenderTargetDesc;
            mainRenderTargetDesc.format          = Graphics::RHIFormat::R11G11B10_Float;
            mainRenderTargetDesc.flags           = TextureFlags::Texture_RenderTarget;
            mainRenderTargetDesc.wrap            = TextureWrap::CLAMP_TO_EDGE;
            mainRenderTargetDesc.minFilter       = TextureFilter::LINEAR;
            mainRenderTargetDesc.magFilter       = TextureFilter::LINEAR;
            mainRenderTargetDesc.generateMipMaps = false;
            mainRenderTargetDesc.samples         = m_MainTextureSamples;
            m_MainTexture                        = Graphics::Texture2D::Create(mainRenderTargetDesc, width, height);
            m_ForwardData.m_DepthTexture         = TextureDepth::Create(width, height, Renderer::GetRenderer()->GetDepthFormat(), m_MainTextureSamples);

            mainRenderTargetDesc.generateMipMaps      = false;
            mainRenderTargetDesc.anisotropicFiltering = false;
            mainRenderTargetDesc.flags                = TextureFlags::Texture_RenderTarget;
            mainRenderTargetDesc.wrap                 = TextureWrap::CLAMP_TO_EDGE;
            mainRenderTargetDesc.minFilter            = TextureFilter::NEAREST;
            mainRenderTargetDesc.magFilter            = TextureFilter::NEAREST;
            mainRenderTargetDesc.samples              = m_MainTextureSamples;
            mainRenderTargetDesc.format               = Graphics::RHIFormat::R32G32B32A32_Float;
            m_NormalTexture                           = Graphics::Texture2D::Create(mainRenderTargetDesc, width, height);

            mainRenderTargetDesc.samples = 1;
            m_NormalResolveTexture       = Graphics::Texture2D::Create(mainRenderTargetDesc, width, height);
            mainRenderTargetDesc.samples = m_MainTextureSamples;

            m_SSAOTexture  = Graphics::Texture2D::Create(mainRenderTargetDesc, width / 2, height / 2);
            m_SSAOTexture1 = Graphics::Texture2D::Create(mainRenderTargetDesc, width / 2, height / 2);
            m_GBufferAllocated = true;
        }

        auto view        = m_CameraTransform->GetWorldMatrix().Inverse();
        const Mat4 proj  = m_Camera->GetGPUProjectionMatrix(); // reversed-Z
        auto projView    = proj * view;

        if(m_DebugRenderEnabled && DebugRenderer::GetInstance())
        {
            DebugRenderer::GetInstance()->SetDimensions(m_MainTexture->GetWidth(), m_MainTexture->GetHeight());
            DebugRenderer::GetInstance()->SetProjView(projView);
        }

        Scene::SceneRenderSettings& renderSettings = scene->GetSettings().RenderSettings;

        // TAA: the 3D camera moves by under a pixel every frame (Halton 2,3 over 8 frames) so the
        // resolve sees each pixel from many positions. Only what the forward set draws is jittered.
        m_TAAViewProj = projView;
        m_TAAJittered = TAAWanted();
        Mat4 forwardProjView = projView;
        if(m_TAAJittered)
        {
            auto halton = [](u32 index, u32 base)
            {
                float f = 1.0f, r = 0.0f;
                for(u32 i = index; i > 0; i /= base)
                {
                    f /= (float)base;
                    r += f * (float)(i % base);
                }
                return r;
            };
            const u32 sample = (m_TAAFrame++ % 8) + 1;
            const float w    = (float)Maths::Max(1u, m_MainTexture->GetWidth());
            const float h    = (float)Maths::Max(1u, m_MainTexture->GetHeight());
            m_TAAJitter      = Vec2((halton(sample, 2) - 0.5f) * 2.0f / w, (halton(sample, 3) - 0.5f) * 2.0f / h);

            Mat4 jitter(1.0f);
            jitter.values[12] = m_TAAJitter.x;
            jitter.values[13] = m_TAAJitter.y;
            forwardProjView   = jitter * projView;
        }
        else
        {
            m_TAAJitter       = Vec2(0.0f);
            m_TAAHistoryValid = false;
        }

        if(renderSettings.Renderer3DEnabled)
        {
            m_ForwardData.m_DescriptorSet[0]->SetUniformBufferData(0, &forwardProjView);
            m_ForwardData.m_DescriptorSet[0]->Update();
        }

        if(renderSettings.SkyboxRenderEnabled || renderSettings.Renderer3DEnabled)
        {
            auto envView = registry.view<Graphics::Environment>();

            if(envView.size() == 0)
            {
                if(m_ForwardData.m_EnvironmentMap != m_DefaultTextureCube)
                {
                    m_ForwardData.m_EnvironmentMap = m_DefaultTextureCube;
                    m_ForwardData.m_IrradianceMap  = m_DefaultTextureCube;

                    // TODO: remove need for this
                    Graphics::DescriptorDesc info {};
                    info.shader                      = m_ForwardData.m_Shader.get();
                    info.layoutIndex                 = 2;
                    m_ForwardData.m_DescriptorSet[2] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(info));

                    m_CubeMap = nullptr;
                    Graphics::DescriptorDesc descriptorDesc {};
                    descriptorDesc.layoutIndex = 0;
                    descriptorDesc.shader      = m_SkyboxShader.get();
                    m_SkyboxDescriptorSet      = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
                }
            }
            else
            {
                // Just use first
                auto& env = envView.get<Graphics::Environment>(envView.front());

                // the sky moved on from what the scene is lit by: bake it again (not every frame - it stalls)
                s_SkyFramesSinceBake++;
                if(env.GetMode() == 1 && env.GetEnvironmentMap() && s_SkyFramesSinceBake > 60)
                {
                    const SkyBakeData now = BuildSkyBake(scene, env.GetParameters());
                    if(SkyBakeChanged(now, s_SkyBaked))
                    {
                        s_SkyLatest      = now;
                        s_SkyLatestValid = true;
                        env.Load();
                    }
                }

                // the skybox always draws the Environment's sky, probe or not (a re-bake frees the old map)
                m_CubeMap = env.GetEnvironmentMap();
                if(m_ActiveProbe < 0 && m_ForwardData.m_EnvironmentMap != env.GetEnvironmentMap())
                {
                    Graphics::DescriptorDesc info    = {};
                    info.shader                      = m_ForwardData.m_Shader.get();
                    info.layoutIndex                 = 2;
                    m_ForwardData.m_DescriptorSet[2] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(info));
                    m_ForwardData.m_EnvironmentMap   = env.GetEnvironmentMap();
                    m_ForwardData.m_IrradianceMap    = env.GetIrradianceMap();
                    const bool viewIrradiance        = false;

                    m_CubeMap = viewIrradiance ? env.GetIrradianceMap() : env.GetEnvironmentMap();
                }
            }

            // a captured probe lights the scene instead of the sky (the skybox still draws the sky)
            {
                // nearest captured probe to the eye
                const Vec3 eye = m_CameraTransform->GetWorldPosition();
                float best     = 1e30f;
                int pick       = -1;
                for(u32 i = 0; i < m_Probes.Size(); i++)
                {
                    if(!m_Probes[i].Filtered)
                        continue;
                    const float d = Maths::Length2(m_Probes[i].Position - eye);
                    if(d < best)
                    {
                        best = d;
                        pick = (int)i;
                    }
                }
                if(pick < 0 && m_ActiveProbe >= 0)
                    m_ForwardData.m_EnvironmentMap = nullptr; // back to the sky next frame
                m_ActiveProbe = pick;
            }
            if(m_ActiveProbe >= 0 && m_ForwardData.m_EnvironmentMap != m_Probes[m_ActiveProbe].Filtered.get())
            {
                Graphics::DescriptorDesc info    = {};
                info.shader                      = m_ForwardData.m_Shader.get();
                info.layoutIndex                 = 2;
                m_ForwardData.m_DescriptorSet[2] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(info));
                m_ForwardData.m_EnvironmentMap   = m_Probes[m_ActiveProbe].Filtered.get();
                m_ForwardData.m_IrradianceMap    = m_Probes[m_ActiveProbe].Irradiance.get();
            }

            // the skybox works the air out per vertex: it needs the sun as well as the view
            struct SkyboxVertexData
            {
                Mat4 InvProjection;
                Mat4 InvView;
                Vec4 SunDirection;
                Vec4 SunColour;
                Vec4 HorizonTint;
                Vec4 ZenithTint;
                Vec4 Params; // x: 1 = procedural sky
            } skyVertex;
            const SkyBakeData sky  = BuildSkyBake(scene, Vec4(0.0f));
            s_SkyLatest            = sky;
            s_SkyLatestValid       = true;
            bool procedural        = false;
            if(envView.size() != 0)
                procedural = envView.get<Graphics::Environment>(envView.front()).GetMode() == 1;
            skyVertex.InvProjection = proj.Inverse();
            skyVertex.InvView       = view.Inverse();
            skyVertex.SunDirection  = sky.SunDirection;
            skyVertex.SunColour     = sky.SunColour;
            skyVertex.HorizonTint   = sky.HorizonTint;
            skyVertex.ZenithTint    = sky.ZenithTint;
            skyVertex.Params        = Vec4(procedural ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f);
            m_SkyboxDescriptorSet->SetUniformBufferData(0, &skyVertex);
        }

        Light* directionaLight = nullptr;
        m_HasSun               = false;
        static GPULight lights[MAX_LIGHTS];
        uint32_t numLights = 0;

        static ShadowData::SpotCandidate spotCandidates[MAX_LIGHTS];
        u32 spotCandidateCount = 0;
        m_ShadowData.m_SpotCount = 0;
        if(renderSettings.Renderer3DEnabled)
            s_SkySunFromLight = false;

        m_ForwardData.m_Frustum = m_Camera->GetFrustum(view);
        UpdateOcclusion(projView, renderSettings.OcclusionCulling && renderSettings.Renderer3DEnabled && m_Probe.Face < 0
                                      && !m_Camera->IsOrthographic() && m_MainTextureSamples == 1);

        auto emitterGroup = registry.group<ParticleEmitter>(entt::get<Maths::Transform>);
        for(auto& emitterEntity : emitterGroup)
        {
            const auto& [emitter, trans] = emitterGroup.get<ParticleEmitter, Maths::Transform>(emitterEntity);
            emitter.Update((float)Engine::GetTimeStep().GetSeconds(), trans.GetWorldPosition());
        }

        m_ForwardData.m_CommandQueue.Clear();
        m_Has3D = false;

        struct UniformSceneData
        {
            GPULight lights[MAX_LIGHTS];
            Mat4 ShadowTransform[SHADOWMAP_MAX];
            Mat4 ViewMatrix;
            Mat4 LightView;
            Mat4 BiasMatrix;
            Vec4 CameraPosition;
            float LightSize;
            float MaxShadowDist;
            float ShadowFade;
            float CascadeFade;
            int LightCount;
            int ShadowCount;
            int Mode;
            int EnvMipCount;
            Vec4 SplitDepths;
            float InitialBias;
            float Width;
            float Height;
            int ShadowEnabled;
            int BlendShadows;
            int PCFSamples;
            int VogelOffset;
            int FilterShadows;
            int TileCountX;
            int TileSize;
            float EnvExposure;
            float ScenePad0;
            Vec4 FogColour;
            Vec4 FogParams;
            Vec4 GIOrigin;
            Vec4 GISpacing;
            i32 GICounts[4];
            Vec4 SceneExtras;
            Mat4 SpotShadowMatrix[SPOT_SHADOW_MAX]; // world -> tile uv + depth
            Vec4 SpotShadowRect[SPOT_SHADOW_MAX];   // xy = tile corner, z = tile size (layer uv), w = array layer
            Vec4 SpotShadowParams[SPOT_SHADOW_MAX]; // x = bias, y = normal bias (texels), z = texel world size 1 m out, w = strength
        };

        if(renderSettings.Renderer3DEnabled)
        {
            {
                LUMOS_PROFILE_SCOPE("Get Light");
                auto group = registry.group<Graphics::Light>(entt::get<Maths::Transform>);

                for(auto& lightEntity : group)
                {
                    if(!Entity(lightEntity, scene).Active())
                        continue;

                    if(numLights >= MAX_LIGHTS)
                        break;

                    const auto& [light, trans] = group.get<Graphics::Light, Maths::Transform>(lightEntity);
                    light.Position             = Vec4(trans.GetWorldPosition(), 1.0f);
                    Vec3 forward               = Vec3(0.0f, 0.0f, 1.0f);
                    forward                    = trans.GetWorldOrientation() * forward;
                    forward.Normalise();
                    light.Direction = Vec4(forward, 1.0f);

                    if(light.Type == (float)Graphics::LightType::DirectionalLight)
                    {
                        directionaLight = &light;
                        // the first one is the sun in the sky - unless it is in made-up units
                        // (a scene lit by its environment alone), which would turn the sky black
                        if(!s_SkySunFromLight && light.Intensity >= kSkyMinSunLux)
                        {
                            s_SkySunFromLight   = true;
                            s_SkySunDirection   = forward;
                            s_SkySunIlluminance = Vec3(light.Colour.x, light.Colour.y, light.Colour.z) * light.Intensity;
                        }
                    }

                    if(light.Type != float(LightType::DirectionalLight))
                    {
                        const bool exactReach = light.Type == (float)LightType::PointLight || renderSettings.LightRange;
                        auto inside = m_ForwardData.m_Frustum.IsInside(Maths::BoundingSphere(Vec3(light.Position), exactReach ? light.Radius : light.Radius * 100));

                        if(inside == Maths::Intersection::OUTSIDE)
                            continue;
                    }

                    lights[numLights] = light.ToGPU();
                    lights[numLights].Intensity *= m_Exposure;
                    lights[numLights].Direction.w = -1.0f; // spot shadow slot, filled in below

                    if(light.CastShadows && light.Type == float(LightType::SpotLight) && light.Intensity > 0.0f && renderSettings.MaxSpotShadows > 0
                       && renderSettings.ShadowsEnabled && Application::Get().GetQualitySettings().EnableShadows)
                    {
                        const float distance = Maths::Length(Vec3(light.Position) - m_CameraTransform->GetWorldPosition());
                        if(distance < renderSettings.SpotShadowDistance)
                        {
                            ShadowData::SpotCandidate& c = spotCandidates[spotCandidateCount++];
                            c.Source         = &light;
                            c.ID             = (u64)(u32)lightEntity;
                            c.Index          = numLights;
                            c.Distance       = distance;
                            c.Far            = SpotShadowFar(light, renderSettings.LightRange);
                            c.Score          = c.Far / Maths::Max(distance, 1.0f);
                            c.Size           = SpotShadowTileSize(light, c.Far, distance, renderSettings.SpotShadowMaxResolution, m_ShadowData.m_ShadowMapSize,
                                                                  m_Camera->IsOrthographic() ? 0.0f : Maths::Tan(m_Camera->GetFOV() * 0.5f), (float)m_MainTexture->GetHeight());
                        }
                    }
                    numLights++;
                }
            }

            if(directionaLight)
            {
                m_HasSun         = true;
                m_SunDirection   = Vec3(directionaLight->Direction.x, directionaLight->Direction.y, directionaLight->Direction.z);
                m_SunIlluminance = Vec3(directionaLight->Colour.x, directionaLight->Colour.y, directionaLight->Colour.z) * directionaLight->Intensity;
            }

            if(renderSettings.ShadowsEnabled && Application::Get().GetQualitySettings().EnableShadows)
            {
                for(uint32_t i = 0; i < m_ShadowData.m_ShadowMapNum; i++)
                {
                    m_ShadowData.m_CascadeCommandQueue[i].Clear();
                }

                if(directionaLight)
                {
                    UpdateCascades(scene, directionaLight);

                    for(uint32_t i = 0; i < m_ShadowData.m_ShadowMapNum; i++)
                    {
                        m_ShadowData.m_CascadeFrustums[i].Define(m_ShadowData.m_ShadowProjView[i]);

                        const Mat4& cm = m_ShadowData.m_ShadowProjView[i];
                        const Vec3 row0(( cm * Vec4(1, 0, 0, 0)).x, (cm * Vec4(0, 1, 0, 0)).x, (cm * Vec4(0, 0, 1, 0)).x);
                        m_ShadowData.m_CascadeTexelWorld[i] = 2.0f / Maths::Max(1e-6f, Maths::Length(row0) * (float)m_ShadowData.m_ShadowMapSize);
                    }
                }

                if(spotCandidateCount > 0)
                    PlanSpotShadows(scene, spotCandidates, spotCandidateCount, lights);
            }

            UniformSceneData uniformSceneData;
            auto& shadowData = GetShadowData();
            MemoryCopy(uniformSceneData.ShadowTransform, shadowData.m_ShadowProjView, sizeof(Mat4) * shadowData.m_ShadowMapNum);
            // Shadows fade out where the last cascade ends, not at the nominal range - past
            // that there is no shadow map to sample and they would cut off in a line.
            const float shadowReach         = shadowData.m_MaxShadowDistance * kShadowFarScale * kLastCascadeSplit;
            uniformSceneData.SplitDepths    = shadowData.m_SplitDepth;
            uniformSceneData.LightView      = shadowData.m_LightMatrix;
            uniformSceneData.InitialBias    = shadowData.m_InitialBias;
            uniformSceneData.LightSize      = shadowData.m_LightSize;
            uniformSceneData.ShadowFade     = Maths::Min(shadowData.m_ShadowFade, shadowReach * 0.25f);
            uniformSceneData.CascadeFade    = shadowData.m_CascadeFade;
            uniformSceneData.MaxShadowDist  = shadowReach;
            uniformSceneData.Width          = (float)m_MainTexture->GetWidth();
            uniformSceneData.Height         = (float)m_MainTexture->GetHeight();
            uniformSceneData.ShadowEnabled  = (renderSettings.ShadowsEnabled && Application::Get().GetQualitySettings().EnableShadows) ? 1 : 0;
            uniformSceneData.ShadowCount    = shadowData.m_ShadowMapNum;
            uniformSceneData.EnvMipCount    = m_ForwardData.m_EnvironmentMap ? m_ForwardData.m_EnvironmentMap->GetMipMapLevels() : 0;
            uniformSceneData.ViewMatrix     = view;
            uniformSceneData.BiasMatrix     = m_ForwardData.m_BiasMatrix;
            uniformSceneData.Mode           = m_ForwardData.m_RenderMode;
            uniformSceneData.CameraPosition = Vec4(m_CameraTransform->GetWorldPosition(), 1.0f);
            uniformSceneData.BlendShadows   = 1;
            uniformSceneData.PCFSamples     = 4;
            uniformSceneData.VogelOffset    = 1;
            uniformSceneData.FilterShadows  = 1;

            switch(Application::Get().GetQualitySettings().ShadowQuality)
            {
            case ShadowQualitySetting::Low:
                uniformSceneData.BlendShadows  = 0;
                uniformSceneData.PCFSamples    = 1;
                uniformSceneData.VogelOffset   = 0;
                uniformSceneData.FilterShadows = 0;
                break;
            case ShadowQualitySetting::Medium:
                uniformSceneData.BlendShadows  = 1;
                uniformSceneData.PCFSamples    = 8;
                uniformSceneData.VogelOffset   = 1;
                uniformSceneData.FilterShadows = 1;
                break;
            case ShadowQualitySetting::High:
                uniformSceneData.BlendShadows  = 1;
                uniformSceneData.PCFSamples    = 16;
                uniformSceneData.VogelOffset   = 1;
                uniformSceneData.FilterShadows = 1;
                break;
            }

            MemoryCopy(uniformSceneData.lights, lights, sizeof(GPULight) * numLights);
            uniformSceneData.LightCount = numLights;
            m_NumLights = numLights;
            m_HasLocalLights = false;
            for(uint32_t i = 0; i < numLights; i++)
                m_HasLocalLights |= lights[i].Type != 0.0f;
            uniformSceneData.TileCountX = m_ForwardPlusEnabled ? (int)m_TileCountX : 0;
            uniformSceneData.TileSize   = m_ForwardPlusEnabled ? (int)m_TileSize : 0;
            uniformSceneData.EnvExposure = m_Exposure * kSkyUnit; // as the skybox draws the same maps
            uniformSceneData.ScenePad0   = renderSettings.LightRange ? 1.0f : 0.0f;

            uniformSceneData.FogColour = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
            uniformSceneData.FogParams = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
            {
                auto envView = registry.view<Graphics::Environment>();
                if(!envView.empty())
                {
                    auto& env = envView.get<Graphics::Environment>(envView.front());
                    uniformSceneData.FogColour = env.GetFogColour();
                    uniformSceneData.FogParams = env.GetFogParams();
                }
            }
            // the volumetric pass draws the fog instead
            if(renderSettings.VolumetricFogEnabled && !m_DisablePostProcess)
                uniformSceneData.FogColour.w = 0.0f;

            // probe GI: the grid is (re)built here, before its buffer is bound below
            ProbeGIParams giParams;
            giParams.Spacing      = renderSettings.GIProbeSpacing;
            m_GI->SetEnabled(renderSettings.GIEnabled);
            m_GI->Prepare(scene, renderSettings.GIEnabled && m_Probe.Face < 0, giParams);
            uniformSceneData.GIOrigin  = Vec4(m_GI->GetOrigin(), m_GI->IsActive() ? 1.0f : 0.0f);
            uniformSceneData.GISpacing = Vec4(m_GI->GetSpacing(), renderSettings.GIIntensity);
            for(int i = 0; i < 3; i++)
                uniformSceneData.GICounts[i] = (i32)m_GI->GetCounts()[i];
            uniformSceneData.GICounts[3] = (i32)m_GI->GetProbeCount();

            // decals: the nearest MAX_DECALS in view, textures shared by slot
            struct DecalGPU
            {
                Mat4 WorldToDecal;
                Vec4 Colour;
                Vec4 Params;
                Vec4 Axis;
            };
            static constexpr u32 kMaxDecals = 32, kMaxDecalTextures = 4; // matches MAX_DECAL_TEXTURES (iOS sampler limit)
            static DecalGPU decals[kMaxDecals];
            Texture* decalTextures[kMaxDecalTextures];
            u32 decalCount = 0, decalTextureCount = 0;
            {
                struct Candidate
                {
                    DecalComponent* Decal;
                    Maths::Transform* Trans;
                    float Distance;
                };
                Candidate candidates[kMaxDecals * 4];
                u32 candidateCount = 0;
                const Vec3 eye     = m_CameraTransform->GetWorldPosition();
                auto decalView     = registry.view<DecalComponent, Maths::Transform>();
                for(auto handle : decalView)
                {
                    if(candidateCount >= kMaxDecals * 4)
                        break;
                    if(!Entity(handle, scene).Active())
                        continue;
                    auto& decal = decalView.get<DecalComponent>(handle);
                    auto& trans = decalView.get<Maths::Transform>(handle);
                    Maths::BoundingBox box(Vec3(-1.0f), Vec3(1.0f));
                    box = box.Transformed(trans.GetWorldMatrix());
                    if(!m_ForwardData.m_Frustum.IsInside(box))
                        continue;
                    candidates[candidateCount++] = { &decal, &trans, Maths::Length2(trans.GetWorldPosition() - eye) };
                }
                // nearest first if there are too many, then by Order so the higher ones blend last
                for(u32 i = 1; i < candidateCount; i++)
                    for(u32 j = i; j > 0 && candidates[j].Distance < candidates[j - 1].Distance; j--)
                        Swap(candidates[j], candidates[j - 1]);
                candidateCount = Maths::Min(candidateCount, kMaxDecals);
                for(u32 i = 1; i < candidateCount; i++)
                    for(u32 j = i; j > 0 && candidates[j].Decal->Order < candidates[j - 1].Decal->Order; j--)
                        Swap(candidates[j], candidates[j - 1]);

                for(u32 i = 0; i < candidateCount; i++)
                {
                    DecalComponent& decal = *candidates[i].Decal;
                    const Mat4& world     = candidates[i].Trans->GetWorldMatrix();
                    int slot              = -1;
                    if(decal.Texture)
                    {
                        for(u32 t = 0; t < decalTextureCount; t++)
                            if(decalTextures[t] == decal.Texture.get())
                                slot = (int)t;
                        if(slot < 0 && decalTextureCount < kMaxDecalTextures)
                        {
                            slot                              = (int)decalTextureCount;
                            decalTextures[decalTextureCount++] = decal.Texture.get();
                        }
                        if(slot < 0)
                            continue; // out of texture slots
                    }
                    Vec3 axis = Vec3(world * Vec4(0.0f, 1.0f, 0.0f, 0.0f));
                    axis.Normalise();
                    DecalGPU& gpu    = decals[decalCount++];
                    gpu.WorldToDecal = world.Inverse();
                    gpu.Colour       = decal.Colour;
                    gpu.Params       = Vec4((float)slot, Maths::Clamp(decal.AngleFade, 0.06f, 1.0f), decal.Roughness, decal.Emissive);
                    gpu.Axis         = Vec4(axis, 0.0f);
                }
            }
            for(u32 t = decalTextureCount; t < kMaxDecalTextures; t++)
                decalTextures[t] = Material::GetDefaultTexture().get();

            const bool contactShadows      = m_ContactShadowsValid && renderSettings.ContactShadowsEnabled;
            uniformSceneData.SceneExtras   = Vec4(contactShadows ? 1.0f : 0.0f, (float)decalCount, 0.0f, 0.0f);

            {
                const float mapSize = (float)m_ShadowData.m_ShadowMapSize;
                for(u32 i = 0; i < m_ShadowData.m_SpotCount; i++)
                {
                    const auto& spot                     = m_ShadowData.m_Spots[i];
                    uniformSceneData.SpotShadowMatrix[i] = m_ForwardData.m_BiasMatrix * spot.ProjView;
                    uniformSceneData.SpotShadowRect[i]   = Vec4(spot.X / mapSize, spot.Y / mapSize, spot.Size / mapSize, float(m_ShadowData.m_ShadowMapNum + spot.Layer));
                    uniformSceneData.SpotShadowParams[i] = Vec4(spot.Bias, spot.NormalBias, spot.TexelWorld, spot.Fade);
                }
            }

            // Upload lights to SSBO for Forward+ culling
            if(m_ForwardPlusEnabled && numLights > 0)
            {
                m_LightSSBO->SetData(numLights * sizeof(GPULight), lights);
            }

            m_ForwardData.m_DescriptorSet[2]->SetUniformBufferData(5, &uniformSceneData);

            // Lazy targets must be full size before this frame's descriptors point at them:
            // resizing later in OnRender left the forward pass reading the 1x1 / one-tile placeholders
            if(!registry.view<ModelComponent>().empty() || !registry.view<VoxelWorldComponent>().empty())
            {
                EnsureShadowTargets();
                if(m_ForwardPlusEnabled && !m_LightIndexAllocated)
                {
                    m_LightIndexAllocated = true;
                    m_StaticGridLights    = ~0u;
                    m_LightIndexSSBO->Resize(m_TileCountX * m_TileCountY * MAX_LIGHTS_PER_TILE * sizeof(uint32_t), nullptr);
                }
            }

            m_ForwardData.m_DescriptorSet[2]->SetTexture(0, reinterpret_cast<Texture*>(shadowData.m_ShadowTex), 0, TextureType::DEPTHARRAY);
            m_ForwardData.m_DescriptorSet[2]->SetTexture(3, m_ForwardData.m_BRDFLUT.get());
            m_ForwardData.m_DescriptorSet[2]->SetTexture(4, m_SSAOValid ? m_SSAOTexture : Material::GetDefaultTexture().get());
            m_ForwardData.m_DescriptorSet[2]->SetTexture(1, m_ForwardData.m_EnvironmentMap, 0, TextureType::CUBE);
            m_ForwardData.m_DescriptorSet[2]->SetTexture(2, m_ForwardData.m_IrradianceMap, 0, TextureType::CUBE);
            m_ForwardData.m_DescriptorSet[2]->SetStorageBuffer(9, m_GI->GetProbeBuffer());
            m_ForwardData.m_DescriptorSet[2]->SetTexture(10, contactShadows ? m_ContactShadowTexture : Material::GetDefaultTexture().get());
            m_ForwardData.m_DescriptorSet[2]->SetUniformBufferData(11, decals);
            m_ForwardData.m_DescriptorSet[2]->SetTexture(12, decalTextures, kMaxDecalTextures);

            // every forward variant declares the Forward+ buffers; without Forward+ something still has to be bound there
            StorageBuffer* placeholder = m_GI->GetProbeBuffer();
            m_ForwardData.m_DescriptorSet[2]->SetStorageBuffer(6, m_LightSSBO ? m_LightSSBO : placeholder);
            m_ForwardData.m_DescriptorSet[2]->SetStorageBuffer(7, m_LightGridSSBO ? m_LightGridSSBO : placeholder);
            m_ForwardData.m_DescriptorSet[2]->SetStorageBuffer(8, m_LightIndexSSBO ? m_LightIndexSSBO : placeholder);

            m_ForwardData.m_DescriptorSet[2]->Update();

            Mat4 boneTransforms[256]; // MAX_BONES in Buffers.glslh
            for(int i = 0; i < 256; i++)
            {
                boneTransforms[i] = Mat4(1.0f);
            }
            m_ForwardData.m_DescriptorSet[3]->SetUniformBufferData(0, boneTransforms);
            m_ForwardData.m_DescriptorSet[3]->Update();

            auto group = registry.group<ModelComponent>(entt::get<Maths::Transform>);

            Graphics::PipelineDesc pipelineDesc = {};
            pipelineDesc.shader                 = m_ForwardData.m_Shader;
            pipelineDesc.polygonMode            = Graphics::PolygonMode::FILL;
            pipelineDesc.blendMode              = BlendMode::SrcAlphaOneMinusSrcAlpha;
            pipelineDesc.clearTargets           = false;
            pipelineDesc.swapchainTarget        = false;

            Graphics::PipelineDesc shadowPipelineDesc;
            // Back-face culling keeps the occluder depth on the caster's lit side, so
            // shadows stay anchored to their caster. Front-face culling would record
            // the far side instead, adding the object's own thickness to the offset -
            // the normal-offset bias in ForwardPBR.frag handles acne instead.
            shadowPipelineDesc.cullMode                = Graphics::CullMode::BACK;
            // every cascade draws into its own layer of the array (BindPipeline(pipeline, layer))
            shadowPipelineDesc.depthArrayTarget        = reinterpret_cast<Texture*>(m_ShadowData.m_ShadowTex);
            shadowPipelineDesc.depthBiasEnabled        = false;
            shadowPipelineDesc.depthBiasConstantFactor = 0.0f;
            shadowPipelineDesc.depthBiasSlopeFactor    = 0.0f;
            shadowPipelineDesc.DebugName               = "Shadow";
            shadowPipelineDesc.clearTargets            = false;


            // Pipelines here only vary by a couple of material flags (and cascade index for
            // shadows), so memoise them - Pipeline::Get hashes the whole desc and hits a map,
            // which is far too much to pay per mesh per cascade.
            SharedPtr<Graphics::Pipeline> shadowPipelines[SHADOWMAP_MAX * 4] = {};
            SharedPtr<Graphics::Pipeline> spotPipelines[SPOT_SHADOW_LAYERS * 4] = {};
            SharedPtr<Graphics::Pipeline> forwardPipelines[32]               = {};
            Material* lastBoundMaterial                                      = nullptr;

            for(auto entity : group)
            {
                if(!Entity(entity, scene).Active())
                    continue;

                const auto& [model, trans] = group.get<ModelComponent, Maths::Transform>(entity);

                if(!model.ModelRef)
                    continue;

                // LOD is picked per mesh from its own world box: a level model is one huge
                // model whose pieces sit at very different distances
                const bool hasLODs = model.ModelRef->GetLODCount() > 1;
                const auto& meshes = model.ModelRef->GetLODMeshes(0);
                const Vec3 camPos  = m_CameraTransform->GetWorldPosition();
                const float tanHalfFov = Maths::Tan(m_Camera->GetFOV() * 0.5f); // Maths::Tan takes degrees
                const float minScreenSize = renderSettings.MinScreenSize;

                for(u32 meshIdx = 0; meshIdx < (u32)meshes.Size(); meshIdx++)
                {
                    auto& worldTransform = trans.GetWorldMatrix();
                    auto bbCopy          = meshes[meshIdx]->GetBoundingBox().Transformed(worldTransform);

                    u32 lod = 0;
                    if(hasLODs)
                    {
                        const float radius = Maths::Length(bbCopy.GetExtents()) * 0.5f;
                        float screenSize   = 1.0f;
                        if(m_Camera->IsOrthographic())
                            screenSize = radius / Maths::Max(0.0001f, m_Camera->GetScale());
                        else
                            screenSize = radius / Maths::Max(0.0001f, Maths::Length(bbCopy.Center() - camPos) * tanHalfFov);
                        lod = model.ModelRef->SelectLOD(screenSize);
                        if(lod > 0)
                            m_Stats.NumLODMeshes++;
                    }
                    const auto& lodMeshes = model.ModelRef->GetLODMeshes(lod);
                    auto& mesh            = meshIdx < (u32)lodMeshes.Size() ? lodMeshes[meshIdx] : meshes[meshIdx];

                    auto instMatIt       = model.InstanceMaterials.find(meshIdx);
                    Material* resolvedMat = instMatIt != model.InstanceMaterials.end()
                        ? instMatIt->second.get()
                        : (mesh->GetMaterial() ? mesh->GetMaterial().get() : m_ForwardData.m_DefaultMaterial);

                    if(directionaLight)
                    {
                        for(uint32_t i = 0; i < m_ShadowData.m_ShadowMapNum; i++)
                        {
                            auto inside = m_ShadowData.m_CascadeFrustums[i].IsInside(bbCopy);

                            if(!inside)
                                continue;

                            // smaller than ~1.5 shadow texels: it would not be seen in the map
                            if(Maths::Length(bbCopy.GetExtents()) < m_ShadowData.m_CascadeTexelWorld[i] * 1.5f)
                                continue;

                            RenderCommand command;
                            command.mesh      = mesh.get();
                            command.transform = worldTransform;
                            command.material  = resolvedMat;

                            if(command.material->GetFlag(Material::RenderFlags::NOSHADOW))
                                continue;

                            Material* material = command.material ? command.material : m_ForwardData.m_DefaultMaterial;
                            bool alphaBlend    = material->GetFlag(Material::RenderFlags::ALPHABLEND);

                            shadowPipelineDesc.transparencyEnabled = alphaBlend;
                            shadowPipelineDesc.shader              = alphaBlend ? m_ShadowData.m_ShaderAlpha : m_ShadowData.m_Shader;

                            // Bind here in case not bound in the loop below as meshes will be inside
                            // cascade frustum and not the cameras
                            if(command.material != lastBoundMaterial)
                            {
                                command.material->Bind();
                                lastBoundMaterial = command.material;
                            }

                            const bool animated = mesh->GetAnimVertexBuffer() != nullptr;
                            if(animated)
                            {
                                shadowPipelineDesc.shader = alphaBlend ? m_ShadowData.m_ShaderAnimAlpha : m_ShadowData.m_ShaderAnim;

                                command.animated              = true;
                                command.AnimatedDescriptorSet = model.Animator ? model.Animator->GetDescriptorSet() : (model.ModelRef->GetAnimationController() ? model.ModelRef->GetAnimationController()->GetDescriptorSet() : m_ForwardData.m_DescriptorSet[3]);
                            }

                            SharedPtr<Graphics::Pipeline>& cached = shadowPipelines[i * 4 + (alphaBlend ? 2 : 0) + (animated ? 1 : 0)];
                            if(!cached)
                            {
                                SetShadowTarget(shadowPipelineDesc, this, m_ShadowData.m_ShadowTex, i, m_ShadowData.m_ShadowMapSize);
                                cached = Graphics::Pipeline::Get(shadowPipelineDesc);
                            }
                            command.pipeline = cached.get();

                            m_ShadowData.m_CascadeCommandQueue[i].PushBack(command);
                        }
                    }

                    for(u32 i = 0; i < m_ShadowData.m_SpotCount; i++)
                    {
                        auto& spot = m_ShadowData.m_Spots[i];
                        if(!spot.Frustum.IsInside(bbCopy) || resolvedMat->GetFlag(Material::RenderFlags::NOSHADOW))
                            continue;

                        // smaller than ~1.5 texels where it stands
                        const float lightDistance = Maths::Length(bbCopy.Center() - Vec3(lights[spot.LightIndex].Position));
                        if(Maths::Length(bbCopy.GetExtents()) < spot.TexelWorld * lightDistance * 1.5f)
                            continue;

                        RenderCommand command;
                        command.mesh       = mesh.get();
                        command.transform  = worldTransform;
                        command.material   = resolvedMat;
                        const bool alphaBlend = resolvedMat->GetFlag(Material::RenderFlags::ALPHABLEND);
                        const bool animated   = mesh->GetAnimVertexBuffer() != nullptr;

                        if(resolvedMat != lastBoundMaterial)
                        {
                            resolvedMat->Bind();
                            lastBoundMaterial = resolvedMat;
                        }

                        shadowPipelineDesc.transparencyEnabled = alphaBlend;
                        shadowPipelineDesc.shader              = animated ? (alphaBlend ? m_ShadowData.m_ShaderAnimAlpha : m_ShadowData.m_ShaderAnim)
                                                                          : (alphaBlend ? m_ShadowData.m_ShaderAlpha : m_ShadowData.m_Shader);
                        if(animated)
                        {
                            command.animated              = true;
                            command.AnimatedDescriptorSet = model.Animator ? model.Animator->GetDescriptorSet() : (model.ModelRef->GetAnimationController() ? model.ModelRef->GetAnimationController()->GetDescriptorSet() : m_ForwardData.m_DescriptorSet[3]);
                            spot.Draw                     = true;
                            spot.OnChange                 = false; // a skinned caster moves every frame
                        }

                        SharedPtr<Graphics::Pipeline>& cached = spotPipelines[spot.Layer * 4 + (alphaBlend ? 2 : 0) + (animated ? 1 : 0)];
                        if(!cached)
                        {
                            SetShadowTarget(shadowPipelineDesc, this, m_ShadowData.m_ShadowTex, m_ShadowData.m_ShadowMapNum + spot.Layer, m_ShadowData.m_ShadowMapSize);
                            cached = Graphics::Pipeline::Get(shadowPipelineDesc);
                        }
                        command.pipeline = cached.get();
                        m_ShadowData.m_SpotCommandQueue[i].PushBack(command);
                    }

                    {
                        // hidden models stop here: the shadow commands above are already queued
                        if(!model.Visible)
                            continue;

                        auto inside = m_ForwardData.m_Frustum.IsInside(bbCopy);

                        if(!inside)
                            continue;

                        // too small on screen to matter (a visibility range by size)
                        if(minScreenSize > 0.0f && !mesh->GetAnimVertexBuffer() && !m_Camera->IsOrthographic())
                        {
                            const float dist = Maths::Length(bbCopy.Center() - camPos);
                            if(Maths::Length(bbCopy.GetExtents()) < minScreenSize * dist * tanHalfFov)
                            {
                                m_Stats.NumSmallCulled++;
                                continue;
                            }
                        }

                        // skinned meshes can leave their rest box, so they always draw
                        if(m_Occlusion.Active && !mesh->GetAnimVertexBuffer() && IsOccluded(bbCopy))
                        {
                            m_Stats.NumOcclusionCulled++;
                            continue;
                        }

                        RenderCommand command;
                        command.mesh      = mesh;
                        command.transform = worldTransform;
                        command.material  = resolvedMat;
                        command.sortDistance = Maths::Length2(bbCopy.Center() - camPos);

                        // Update material buffers
                        if(command.material != lastBoundMaterial)
                        {
                            command.material->Bind();
                            lastBoundMaterial = command.material;
                        }

                        pipelineDesc.colourTargets[0]    = m_MainTexture;
                        pipelineDesc.cullMode            = command.material->GetFlag(Material::RenderFlags::TWOSIDED) ? Graphics::CullMode::NONE : Graphics::CullMode::BACK;
                        pipelineDesc.transparencyEnabled = command.material->GetFlag(Material::RenderFlags::ALPHABLEND);
                        pipelineDesc.samples             = m_MainTextureSamples;
                        const bool wireframe             = (scene->GetSettings().RenderSettings.Wireframe || command.material->GetFlag(Material::RenderFlags::WIREFRAME))
                            && Renderer::GetCapabilities().FillModeNonSolid;
                        pipelineDesc.polygonMode         = wireframe ? PolygonMode::LINE : PolygonMode::FILL;
                        if(m_MainTextureSamples > 1)
                            pipelineDesc.resolveTexture = m_ResolveTexture;
                        if(m_ForwardData.m_DepthTest && command.material->GetFlag(Material::RenderFlags::DEPTHTEST))
                        {
                            pipelineDesc.depthTarget = m_ForwardData.m_DepthTexture;
                        }

                        if(mesh->GetAnimVertexBuffer())
                        {
                            pipelineDesc.shader           = m_ForwardData.m_AnimShader;
                            command.animated              = true;
                            command.AnimatedDescriptorSet = model.Animator ? model.Animator->GetDescriptorSet() : (model.ModelRef->GetAnimationController() ? model.ModelRef->GetAnimationController()->GetDescriptorSet() : m_ForwardData.m_DescriptorSet[3]);
                        }
                        else
                            pipelineDesc.shader = m_ForwardData.m_Shader;
#ifndef LUMOS_PRODUCTION
                        static const char* debugName0 = "Forward PBR Transparent DepthTested";
                        static const char* debugName1 = "Forward PBR DepthTested";
                        static const char* debugName2 = "Forward PBR Transparent";
                        static const char* debugName3 = "Forward PBR";

                        if(pipelineDesc.depthTarget && pipelineDesc.transparencyEnabled)
                        {
                            pipelineDesc.DebugName = debugName0;
                        }
                        else if(pipelineDesc.depthTarget)
                        {
                            pipelineDesc.DebugName = debugName1;
                        }
                        else if(pipelineDesc.transparencyEnabled)
                        {
                            pipelineDesc.DebugName = debugName2;
                        }
                        else
                        {
                            pipelineDesc.DebugName = debugName3;
                        }
#endif

                        const u32 pipelineKey = (wireframe ? 16u : 0u)
                            | (pipelineDesc.cullMode == Graphics::CullMode::NONE ? 8u : 0u)
                            | (pipelineDesc.transparencyEnabled ? 4u : 0u)
                            | (pipelineDesc.depthTarget ? 2u : 0u)
                            | (command.animated ? 1u : 0u);

                        SharedPtr<Graphics::Pipeline>& cached = forwardPipelines[pipelineKey];
                        if(!cached)
                            cached = Graphics::Pipeline::Get(pipelineDesc);
                        command.pipeline = cached.get();
                        m_ForwardData.m_CommandQueue.PushBack(command);
                    }
                }
            }
        }

        {
            auto voxelView = registry.view<VoxelWorldComponent>();
            if(voxelView.begin() != voxelView.end())
            {
                const Vec3 camWorldPos = m_CameraTransform->GetWorldPosition();

                Graphics::PipelineDesc voxelPipelineDesc = {};
                voxelPipelineDesc.shader                 = m_ForwardData.m_Shader;
                voxelPipelineDesc.polygonMode            = Graphics::PolygonMode::FILL;
                voxelPipelineDesc.blendMode              = BlendMode::SrcAlphaOneMinusSrcAlpha;
                voxelPipelineDesc.clearTargets           = false;
                voxelPipelineDesc.swapchainTarget        = false;

                Graphics::PipelineDesc voxelShadowDesc;
                voxelShadowDesc.cullMode                = Graphics::CullMode::BACK;
                voxelShadowDesc.depthBiasEnabled        = true;
                voxelShadowDesc.depthBiasConstantFactor = 1.25f;
                voxelShadowDesc.depthBiasSlopeFactor    = 2.75f;
                voxelShadowDesc.DebugName               = "Shadow";
                voxelShadowDesc.clearTargets            = false;
                voxelShadowDesc.depthArrayTarget        = reinterpret_cast<Texture*>(m_ShadowData.m_ShadowTex);

                for(auto entity : voxelView)
                {
                    if(!Entity(entity, scene).Active())
                        continue;

                    auto& vwc = voxelView.get<VoxelWorldComponent>(entity);
                    if(!vwc.World)
                        continue;

                    vwc.World->Update(camWorldPos); // load/evict + re-mesh dirty chunks

                    for(auto& [key, chunk] : vwc.World->GetChunks())
                    {
                        if(!chunk->GetVertexBuffer())
                            continue; // fully-air chunk, nothing meshed

                        Mat4 worldTransform = Mat4::Translation(Graphics::VoxelWorld::ChunkWorldOrigin(chunk->GetChunkX(), chunk->GetChunkZ()));
                        auto bbCopy         = chunk->GetBoundingBox().Transformed(worldTransform);
                        Material* mat       = chunk->GetMaterial() ? chunk->GetMaterial().get() : m_ForwardData.m_DefaultMaterial;

                        if(directionaLight && !mat->GetFlag(Material::RenderFlags::NOSHADOW))
                        {
                            for(uint32_t i = 0; i < m_ShadowData.m_ShadowMapNum; i++)
                            {
                                if(!m_ShadowData.m_CascadeFrustums[i].IsInside(bbCopy))
                                    continue;

                                RenderCommand command;
                                command.mesh         = chunk;
                                command.transform    = worldTransform;
                                command.material     = mat;
                                voxelShadowDesc.shader      = m_ShadowData.m_Shader;
                                SetShadowTarget(voxelShadowDesc, this, m_ShadowData.m_ShadowTex, i, m_ShadowData.m_ShadowMapSize);
                                mat->Bind();
                                command.pipeline = Graphics::Pipeline::Get(voxelShadowDesc);
                                m_ShadowData.m_CascadeCommandQueue[i].PushBack(command);
                            }
                        }

                        if(!m_ForwardData.m_Frustum.IsInside(bbCopy))
                            continue;

                        RenderCommand command;
                        command.mesh      = chunk;
                        command.transform = worldTransform;
                        command.material  = mat;
                        command.sortDistance = Maths::Length2(bbCopy.Center() - m_CameraTransform->GetWorldPosition());
                        mat->Bind();

                        voxelPipelineDesc.colourTargets[0]    = m_MainTexture;
                        voxelPipelineDesc.cullMode            = mat->GetFlag(Material::RenderFlags::TWOSIDED) ? Graphics::CullMode::NONE : Graphics::CullMode::BACK;
                        voxelPipelineDesc.transparencyEnabled = mat->GetFlag(Material::RenderFlags::ALPHABLEND);
                        voxelPipelineDesc.samples             = m_MainTextureSamples;
                        if(m_MainTextureSamples > 1)
                            voxelPipelineDesc.resolveTexture = m_ResolveTexture;
                        if(m_ForwardData.m_DepthTest && mat->GetFlag(Material::RenderFlags::DEPTHTEST))
                            voxelPipelineDesc.depthTarget = m_ForwardData.m_DepthTexture;
                        voxelPipelineDesc.shader   = m_ForwardData.m_Shader;
                        voxelPipelineDesc.DebugName = "Forward PBR Voxel";

                        command.pipeline = Graphics::Pipeline::Get(voxelPipelineDesc);
                        m_ForwardData.m_CommandQueue.PushBack(command);
                    }
                }
            }
        }

        // A spot set to redraw on change keeps last frame's tile when nothing in it moved
        if(m_ShadowData.m_SpotCount > 0)
        {
            CascadeSchedule& schedule = CascadeScheduleFor(this);
            for(u32 i = 0; i < m_ShadowData.m_SpotCount; i++)
            {
                auto& spot = m_ShadowData.m_Spots[i];
                u64 hash   = 14695981039346656037ull;
                hash       = HashBytes(hash, &spot.ProjView, sizeof(Mat4));
                for(const RenderCommand& command : m_ShadowData.m_SpotCommandQueue[i])
                {
                    hash = HashBytes(hash, &command.mesh, sizeof(command.mesh));
                    hash = HashBytes(hash, &command.transform, sizeof(Mat4));
                }

                bool keep = false;
                if(spot.OnChange)
                    for(const SpotTileCache& tile : schedule.Spots)
                        if(tile.Valid && tile.LightID == spot.LightID && tile.Hash == hash && tile.Layer == spot.Layer && tile.X == spot.X && tile.Y == spot.Y && tile.Size == spot.Size)
                            keep = true;
                spot.Draw = !keep;
                spot.Hash = hash;
            }
        }

        // Instance batching: group consecutive commands with same mesh+material
        m_Has3D = !m_ForwardData.m_CommandQueue.Empty();
        if(m_ForwardData.m_InstancedShader && m_ForwardData.m_CommandQueue.Size() > 1)
        {
            auto& queue = m_ForwardData.m_CommandQueue;
            queue.Sort([](const RenderCommand& a, const RenderCommand& b)
            {
                bool aCanInstance = !a.animated && a.material && !a.material->GetFlag(Material::RenderFlags::ALPHABLEND);
                bool bCanInstance = !b.animated && b.material && !b.material->GetFlag(Material::RenderFlags::ALPHABLEND);

                if(aCanInstance != bCanInstance)
                    return aCanInstance;

                if(!aCanInstance)
                    return false;

                if(a.mesh != b.mesh)
                    return a.mesh < b.mesh;
                return InstanceMaterialKey(a.material) < InstanceMaterialKey(b.material);
            });

            // Build instance groups. Scratch buffers persist across frames so the batching
            // pass doesn't malloc/free the whole command queue every frame.
            static TDArray<ForwardInstanceData> instanceData;
            static CommandQueue batchedQueue;
            instanceData.Clear();
            instanceData.Reserve(MAX_INSTANCE_COUNT);
            batchedQueue.Clear();
            batchedQueue.Reserve(queue.Size());

            u32 i = 0;
            while(i < queue.Size())
            {
                auto& cmd = queue[i];

                // Skip animated or transparent — can't instance these
                bool canInstance = !cmd.animated && cmd.material && !cmd.material->GetFlag(Material::RenderFlags::ALPHABLEND);

                if(!canInstance)
                {
                    batchedQueue.PushBack(cmd);
                    i++;
                    continue;
                }

                const u64 cmdKey = InstanceMaterialKey(cmd.material);
                u32 groupStart   = i;
                u32 groupEnd     = i + 1;
                while(groupEnd < queue.Size()
                      && queue[groupEnd].mesh == cmd.mesh
                      && InstanceMaterialKey(queue[groupEnd].material) == cmdKey
                      && !queue[groupEnd].animated
                      && groupEnd - groupStart < MAX_INSTANCE_COUNT)
                {
                    groupEnd++;
                }

                u32 groupSize = groupEnd - groupStart;

                if(groupSize > 1)
                {
                    u32 firstInstance = (u32)instanceData.Size();
                    for(u32 j = groupStart; j < groupEnd; j++)
                    {
                        Vec4 colour = queue[j].material && queue[j].material->GetProperties()
                            ? queue[j].material->GetProperties()->albedoColour
                            : Vec4(1.0f);
                        instanceData.PushBack({ queue[j].transform, colour });
                    }

                    RenderCommand instancedCmd = cmd;
                    instancedCmd.instanced     = true;
                    instancedCmd.instanceCount = groupSize;
                    instancedCmd.firstInstance = firstInstance;
                    batchedQueue.PushBack(instancedCmd);

                    m_Stats.NumInstancedObjects += groupSize;
                    if(m_Stats.NumInstanceBatches < SceneRendererStats::MaxTrackedBatches)
                    {
                        auto& bi = m_Stats.InstanceBatches[m_Stats.NumInstanceBatches];
                        bi.meshName = cmd.mesh ? cmd.mesh->GetName().c_str() : "Unknown";
                        bi.instanceCount = groupSize;
                    }
                    m_Stats.NumInstanceBatches++;
                }
                else
                {
                    batchedQueue.PushBack(cmd);
                }

                i = groupEnd;
            }

            // Upload instance data (transform + colour) to SSBO
            if(instanceData.Size() > 0)
            {
                u32 uploadSize = (u32)(instanceData.Size() * sizeof(ForwardInstanceData));
                if(instanceData.Size() > MAX_INSTANCE_COUNT)
                {
                    uploadSize = MAX_INSTANCE_COUNT * sizeof(ForwardInstanceData);
                    LWARN("Instance count %u exceeds max %u, clamping", (u32)instanceData.Size(), MAX_INSTANCE_COUNT);
                }
                u32 frameIdx = Renderer::GetMainSwapChain()->GetCurrentBufferIndex();
                m_ForwardData.m_InstanceTransformSSBO[frameIdx]->SetData(uploadSize, instanceData.Data());
                m_ForwardData.m_InstanceDescriptorSet[frameIdx]->SetStorageBuffer(0, m_ForwardData.m_InstanceTransformSSBO[frameIdx]);
                m_ForwardData.m_InstanceDescriptorSet[frameIdx]->Update();
            }

            // Swap rather than move-assign: both buffers stay alive for next frame.
            CommandQueue previous = std::move(queue);
            queue                 = std::move(batchedQueue);
            batchedQueue          = std::move(previous);
        }

        // Shadow instance batching
        if(m_ShadowData.m_ShaderInstanced)
        {
            static TDArray<Mat4> shadowInstanceTransforms;
            static CommandQueue shadowBatchedQueue;
            shadowInstanceTransforms.Clear();
            shadowInstanceTransforms.Reserve(MAX_INSTANCE_COUNT);

            for(uint32_t cascadeIdx = 0; cascadeIdx < m_ShadowData.m_ShadowMapNum; cascadeIdx++)
            {
                auto& queue = m_ShadowData.m_CascadeCommandQueue[cascadeIdx];
                if(queue.Size() <= 1)
                    continue;

                queue.Sort([](const RenderCommand& a, const RenderCommand& b)
                    {
                        bool aCanInstance = !a.animated && a.material && !a.material->GetFlag(Material::RenderFlags::ALPHABLEND);
                        bool bCanInstance = !b.animated && b.material && !b.material->GetFlag(Material::RenderFlags::ALPHABLEND);

                        if(aCanInstance != bCanInstance)
                            return aCanInstance;

                        if(!aCanInstance)
                            return false;

                        if(a.mesh != b.mesh)
                            return a.mesh < b.mesh;
                        return a.material < b.material;
                    });

                CommandQueue& batchedQueue = shadowBatchedQueue;
                batchedQueue.Clear();
                batchedQueue.Reserve(queue.Size());

                u32 ci = 0;
                while(ci < queue.Size())
                {
                    auto& cmd = queue[ci];
                    bool canInstance = !cmd.animated && cmd.material && !cmd.material->GetFlag(Material::RenderFlags::ALPHABLEND);

                    if(!canInstance)
                    {
                        batchedQueue.PushBack(cmd);
                        ci++;
                        continue;
                    }

                    u32 groupStart = ci;
                    u32 groupEnd   = ci + 1;
                    while(groupEnd < queue.Size()
                          && queue[groupEnd].mesh == cmd.mesh
                          && queue[groupEnd].material == cmd.material
                          && !queue[groupEnd].animated
                          && groupEnd - groupStart < MAX_INSTANCE_COUNT)
                    {
                        groupEnd++;
                    }

                    u32 groupSize = groupEnd - groupStart;
                    if(groupSize > 1)
                    {
                        u32 firstInstance = (u32)shadowInstanceTransforms.Size();
                        for(u32 j = groupStart; j < groupEnd; j++)
                            shadowInstanceTransforms.PushBack(queue[j].transform);

                        RenderCommand instancedCmd = cmd;
                        instancedCmd.instanced     = true;
                        instancedCmd.instanceCount = groupSize;
                        instancedCmd.firstInstance = firstInstance;
                        batchedQueue.PushBack(instancedCmd);
                    }
                    else
                    {
                        batchedQueue.PushBack(cmd);
                    }

                    ci = groupEnd;
                }

                CommandQueue previous = std::move(queue);
                queue                 = std::move(batchedQueue);
                batchedQueue          = std::move(previous);
            }

            if(shadowInstanceTransforms.Size() > 0)
            {
                u32 uploadSize = (u32)(shadowInstanceTransforms.Size() * sizeof(Mat4));
                if(shadowInstanceTransforms.Size() > MAX_INSTANCE_COUNT)
                    uploadSize = MAX_INSTANCE_COUNT * sizeof(Mat4);
                u32 frameIdx = Renderer::GetMainSwapChain()->GetCurrentBufferIndex();
                m_ShadowData.m_InstanceTransformSSBO[frameIdx]->SetData(uploadSize, shadowInstanceTransforms.Data());
                m_ShadowData.m_InstanceDescriptorSet[frameIdx]->SetStorageBuffer(0, m_ShadowData.m_InstanceTransformSSBO[frameIdx]);
                m_ShadowData.m_InstanceDescriptorSet[frameIdx]->Update();
            }
        }

        if(renderSettings.Renderer3DEnabled && !m_ForwardData.m_CommandQueue.Empty())
        {
            LUMOS_PROFILE_SCOPE("Sort Meshes by distance from camera");
            auto camPos = m_CameraTransform->GetWorldPosition();
            Algorithms::IntroSort(m_ForwardData.m_CommandQueue.begin(), m_ForwardData.m_CommandQueue.end(),
                                  [camPos](RenderCommand& a, RenderCommand& b)
                                  {
                                      if(a.material->GetFlag(Material::RenderFlags::DEPTHTEST) && !b.material->GetFlag(Material::RenderFlags::DEPTHTEST))
                                          return true;
                                      if(!a.material->GetFlag(Material::RenderFlags::DEPTHTEST) && b.material->GetFlag(Material::RenderFlags::DEPTHTEST))
                                          return false;

                                      // box centres: a level model's meshes all share the entity's transform
                                      return a.sortDistance < b.sortDistance;
                                  });
        }

        m_Renderer2DData.m_CommandQueue2D.Clear();
        m_LitData.m_CommandQueue2D.Clear();

        if(renderSettings.Renderer2DEnabled)
        {
            // Gather 2D lights into the packed UBO. Position/Direction are pulled from the Transform.
            int numLights2D            = 0;
            m_Lights2D.ambient         = Vec4(0.0f);
            {
                LUMOS_PROFILE_SCOPE("Get 2D Lights");
                auto lightGroup = registry.group<Graphics::Light2D>(entt::get<Maths::Transform>);
                for(auto lightEntity : lightGroup)
                {
                    if(numLights2D >= MAX_2D_LIGHTS)
                        break;
                    if(!Entity(lightEntity, scene).Active())
                        continue;

                    const auto& [light, trans] = lightGroup.get<Graphics::Light2D, Maths::Transform>(lightEntity);
                    Vec3 worldPos              = trans.GetWorldPosition();
                    Vec3 forward         = trans.GetWorldOrientation() * Vec3(0.0f, -1.0f, 0.0f);

                    light.Position  = Vec4(worldPos.x, worldPos.y, 0.0f, 1.0f);
                    light.Direction = Vec4(forward.x, forward.y, 0.0f, 0.0f);

                    Light2DGPU& g = m_Lights2D.lights[numLights2D];
                    g.Colour      = light.Colour;
                    g.Position    = Vec4(worldPos.x, worldPos.y, 0.0f, light.Radius);
                    g.Direction   = Vec4(forward.x, forward.y, light.Height, light.Type);
                    g.Params      = Vec4(light.Intensity, light.InnerAngle, light.OuterAngle, light.Falloff);
                    numLights2D++;
                }
            }
            m_Lights2D.counts[0] = numLights2D;
            const bool hasLights = m_LitDataInitialised && m_LitData.m_Shader && numLights2D > 0;

            auto routeSprite = [&](Renderable2D* renderable, const Mat4& worldMat)
            {
                RenderCommand2D command;
                command.renderable = renderable;
                command.transform  = worldMat;
                if(hasLights && renderable->ReceivesLight)
                    m_LitData.m_CommandQueue2D.PushBack(command);
                else
                    m_Renderer2DData.m_CommandQueue2D.PushBack(command);
            };

            auto spriteGroup = registry.group<Graphics::Sprite>(entt::get<Maths::Transform>);
            for(auto entity : spriteGroup)
            {
                const auto& [sprite, trans] = spriteGroup.get<Graphics::Sprite, Maths::Transform>(entity);

                auto bb = Maths::BoundingBox(Maths::Rect(sprite.GetPosition(), sprite.GetScale()));
                bb.Transform(trans.GetWorldMatrix());
                auto inside = m_ForwardData.m_Frustum.IsInside(bb);

                if(!inside)
                    continue;

                routeSprite(&sprite, trans.GetWorldMatrix());
            };

            auto animSpriteGroup = registry.group<Graphics::AnimatedSprite>(entt::get<Maths::Transform>);
            for(auto entity : animSpriteGroup)
            {
                const auto& [sprite, trans] = animSpriteGroup.get<Graphics::AnimatedSprite, Maths::Transform>(entity);

                auto bb = Maths::BoundingBox(Maths::Rect(sprite.GetPosition(), sprite.GetScale()));
                bb.Transform(trans.GetWorldMatrix());
                auto inside = m_ForwardData.m_Frustum.IsInside(bb);

                if(!inside)
                    continue;

                routeSprite(&sprite, trans.GetWorldMatrix());
            };

            {
                LUMOS_PROFILE_SCOPE("Sort sprites by z value");
                auto backToFront = [](RenderCommand2D& a, RenderCommand2D& b)
                {
                    const float az = a.transform.Translation()[2];
                    const float bz = b.transform.Translation()[2];
                    if(az != bz)
                        return az < bz;
                    return a.renderable < b.renderable;
                };

                if(!m_Renderer2DData.m_CommandQueue2D.Empty())
                    Algorithms::IntroSort(m_Renderer2DData.m_CommandQueue2D.begin(), m_Renderer2DData.m_CommandQueue2D.end(), backToFront);

                if(!m_LitData.m_CommandQueue2D.Empty())
                    Algorithms::IntroSort(m_LitData.m_CommandQueue2D.begin(), m_LitData.m_CommandQueue2D.end(), backToFront);
            }
        }
    }

    void SceneRenderer::SetRenderTarget(Graphics::Texture* texture, bool onlyIfTargetsScreen, bool rebuildFramebuffer)
    {
        LUMOS_PROFILE_FUNCTION();
        m_ForwardData.m_RenderTexture = texture;
    }

    void SceneRenderer::OnRender()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("Render Passes");

        auto& sceneRenderSettings = Application::Get().GetCurrentScene()->GetSettings().RenderSettings;
        const QualitySettings& qualitySettings         = Application::Get().GetQualitySettings();

        CosmoBake();

        {
            LUMOS_PROFILE_GPU("Clear Main Texture Pass");
            const Vec4 clear = m_TransparentBackground ? Vec4(0.0f) : Vec4(0.1f, 0.1f, 0.1f, 1.0f);
            Renderer::GetRenderer()->ClearRenderTarget(m_MainTexture, Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), clear);

            if(sceneRenderSettings.MSAASamples > 1)
                Renderer::GetRenderer()->ClearRenderTarget(m_ResolveTexture, Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), clear);
        }

        // no 3D geometry: prepass / SSAO / SSR / shadows have nothing to work on
        const bool needGBuffer = m_Has3D && ((sceneRenderSettings.SSAOEnabled && qualitySettings.EnableSSAO)
            || (sceneRenderSettings.SSREnabled && qualitySettings.EnableSSR));
        const bool wantContactShadows = m_Has3D && sceneRenderSettings.ContactShadowsEnabled && sceneRenderSettings.Renderer3DEnabled && !m_DisablePostProcess
            && m_MainTextureSamples == 1 && m_HasSun && m_ContactShadowDescriptorSet;
        // contact shadows march the pre-pass depth before the forward pass needs them
        const bool runDepthPrePass = m_Has3D && ((sceneRenderSettings.DepthPrePass && qualitySettings.EnableDepthPrePass) || needGBuffer || wantContactShadows);
        if(runDepthPrePass || needGBuffer)
            EnsureGBufferTargets();
        if(m_MainTextureSamples > 1)
            EnsureResolveTarget();

        if(needGBuffer)
        {
            LUMOS_PROFILE_GPU("Clear Normal Texture Pass");
            Renderer::GetRenderer()->ClearRenderTarget(m_NormalTexture, Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), Vec4(0.0f));
        }


        // Reset bloom texture; BloomPass sets this if it succeeds
        m_BloomTextureLastRenderered = nullptr;

        if(m_ForwardData.m_DepthTest)
        {
            LUMOS_PROFILE_GPU("Clear Depth Texture Pass");
            // reversed-Z: far is 0
            Renderer::GetRenderer()->ClearRenderTarget(reinterpret_cast<Texture*>(m_ForwardData.m_DepthTexture), Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), Vec4(0.0f));
        }

        GenerateBRDFLUTPass();

        for(RenderPassRecord& record : m_PassRecords)
            record = RenderPassRecord();

        auto runPass = [this](RenderPassID id, void (SceneRenderer::*pass)())
        {
            if(!BeginPass(id))
                return false;
            (this->*pass)();
            EndPass(id);
            return true;
        };

        if(runDepthPrePass)
        {
            runPass(RenderPass_DepthPrePass, &SceneRenderer::DepthPrePass);
            OcclusionGridPass();
        }

        m_SSAOValid = false;
        if(sceneRenderSettings.SSAOEnabled && !m_DisablePostProcess && qualitySettings.EnableSSAO && m_Has3D)
        {
            runPass(RenderPass_SSAO, &SceneRenderer::SSAOPass);

            if(sceneRenderSettings.SSAOBlur)
                runPass(RenderPass_SSAOBlur, &SceneRenderer::SSAOBlurPass);
        }

        m_ContactShadowsValid = false;
        if(wantContactShadows)
        {
            if(!m_ContactShadowsAllocated)
            {
                m_ContactShadowsAllocated = true;
                m_ContactShadowTexture->Resize(m_TargetWidth, m_TargetHeight);
            }
            runPass(RenderPass_ContactShadows, &SceneRenderer::ContactShadowPass);
        }

        if(sceneRenderSettings.ShadowsEnabled && sceneRenderSettings.Renderer3DEnabled && qualitySettings.EnableShadows && m_Has3D)
        {
            EnsureShadowTargets();
            runPass(RenderPass_Shadow, &SceneRenderer::ShadowPass);
        }

        if(m_GI->IsActive() && sceneRenderSettings.Renderer3DEnabled)
            runPass(RenderPass_GI, &SceneRenderer::GIPass);

        if(m_ForwardPlusEnabled && sceneRenderSettings.Renderer3DEnabled && m_Has3D)
        {
            if(!m_LightIndexAllocated)
            {
                m_LightIndexAllocated = true;
                m_StaticGridLights    = ~0u;
                m_LightIndexSSBO->Resize(m_TileCountX * m_TileCountY * MAX_LIGHTS_PER_TILE * sizeof(uint32_t), nullptr);
            }
            s_CullDepthValid = runDepthPrePass;
            runPass(RenderPass_LightCulling, &SceneRenderer::LightCullingPass);
        }

        if(sceneRenderSettings.Renderer3DEnabled)
            runPass(RenderPass_Forward, &SceneRenderer::ForwardPass);
        // no pre-pass: the forward pass's depth feeds the occlusion grid instead
        if(!runDepthPrePass && sceneRenderSettings.Renderer3DEnabled)
            OcclusionGridPass();

        // Nebula backdrop behind everything (no-op unless enabled via Lua).
        runPass(RenderPass_Cosmo, &SceneRenderer::CosmoPass);

        if(sceneRenderSettings.SkyboxRenderEnabled && !m_TransparentBackground)
            runPass(RenderPass_Skybox, &SceneRenderer::SkyboxPass);

        bool probeFrame = false;
        if(m_Probe.Face >= 0)
        {
            ProbeFacePass();
            // the probe's view is not for the screen: show black under the loading UI instead
            Vec4 black(0.0f, 0.0f, 0.0f, 1.0f);
            Renderer::GetRenderer()->ClearRenderTarget(m_MainTextureSamples > 1 ? m_ResolveTexture : m_MainTexture, Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), black);
            probeFrame = true;
        }

        if(sceneRenderSettings.Renderer2DEnabled)
        {
            runPass(RenderPass_2D, &SceneRenderer::Render2DPass);
            runPass(RenderPass_2DLit, &SceneRenderer::Render2DLitPass);
        }

        if(sceneRenderSettings.Renderer3DEnabled)
            runPass(RenderPass_Particle, &SceneRenderer::ParticlePass);

        runPass(RenderPass_PointCloud, &SceneRenderer::PointCloudPass);
        // Instanced anti-aliased line ribbons (constellations, graphs, paths).
        // These three draw queues the game refills every frame. A pass that is cut off
        // still has to empty its queue, or it grows until the pass next runs.
        if(!runPass(RenderPass_LineCloud, &SceneRenderer::LineCloudPass))
            Graphics::ResetImmediateLineClouds();
        // Procedural planet impostors (lit spheres + atmosphere), atop the field.
        if(!runPass(RenderPass_Planet, &SceneRenderer::PlanetPass))
            Graphics::ClearPlanets();

        m_LastRenderTarget = m_MainTextureSamples > 1 ? m_ResolveTexture : m_MainTexture;

        if(sceneRenderSettings.SSREnabled && qualitySettings.EnableSSR && !m_DisablePostProcess && m_Has3D)
            runPass(RenderPass_SSR, &SceneRenderer::SSRPass);

        if(sceneRenderSettings.VolumetricFogEnabled && !m_DisablePostProcess && m_VolumetricFogDescriptorSet && m_VolumetricFogApplyDescriptorSet && !probeFrame)
            runPass(RenderPass_VolumetricFog, &SceneRenderer::VolumetricFogPass);

        // resolves before depth of field / bloom / tone mapping, which want a stable picture
        if(!(m_TAAJittered && !probeFrame && runPass(RenderPass_TAA, &SceneRenderer::TAAPass)))
            m_TAAHistoryValid = false;

        if(sceneRenderSettings.DepthOfFieldEnabled && !m_DisablePostProcess)
            runPass(RenderPass_DepthOfField, &SceneRenderer::DepthOfFieldPass);

        if(sceneRenderSettings.BloomEnabled && !m_DisablePostProcess && qualitySettings.EnableBloom)
        {
            EnsureBloomTargets();
            runPass(RenderPass_Bloom, &SceneRenderer::BloomPass);
        }

        // these write alpha = 1, which would fill a transparent window
        const bool keepsAlpha = !m_TransparentBackground;

        // no separate debanding pass: tone mapping dithers its output (in display space, where banding shows)

        if(!probeFrame) // a black capture frame would push the exposure up
            runPass(RenderPass_EyeAdaptation, &SceneRenderer::EyeAdaptationPass);

        // a frame cut before here is still HDR; tone map it anyway so it is viewable
        if(!runPass(RenderPass_ToneMapping, &SceneRenderer::ToneMappingPass) && m_PassPreviewTonemap)
            ToneMappingPass();

        if(!(sceneRenderSettings.MotionBlurEnabled && !m_DisablePostProcess && keepsAlpha
               && runPass(RenderPass_MotionBlur, &SceneRenderer::MotionBlurPass)))
            m_HasPrevViewProj = false; // reset so re-enable doesn't smear

        if(sceneRenderSettings.VignetteEnabled && !m_DisablePostProcess)
            runPass(RenderPass_Vignette, &SceneRenderer::VignettePass);

        if(sceneRenderSettings.SharpenEnabled && !m_DisablePostProcess)
            runPass(RenderPass_Sharpen, &SceneRenderer::SharpenPass);

        // SMAA takes priority over FXAA when both are set.
        if(sceneRenderSettings.SMAAEnabled && !m_DisablePostProcess && qualitySettings.EnableFXAA && keepsAlpha)
        {
            EnsureSMAATargets();
            runPass(RenderPass_SMAA, &SceneRenderer::SMAAPass);
        }
        else if(sceneRenderSettings.FXAAEnabled && !m_DisablePostProcess && qualitySettings.EnableFXAA && keepsAlpha)
            runPass(RenderPass_FXAA, &SceneRenderer::FXAAPass);

        if(sceneRenderSettings.ChromaticAberationEnabled && !m_DisablePostProcess && keepsAlpha)
            runPass(RenderPass_ChromaticAberation, &SceneRenderer::ChromaticAberationPass);

        if(sceneRenderSettings.FilmicGrainEnabled && !m_DisablePostProcess && keepsAlpha)
            runPass(RenderPass_FilmicGrain, &SceneRenderer::FilmicGrainPass);

        if(m_DebugRenderEnabled && (sceneRenderSettings.DebugRenderEnabled || DebugRenderer::PassRequested()))
            runPass(RenderPass_Debug, &SceneRenderer::DebugPass);

        runPass(RenderPass_Text, &SceneRenderer::TextPass);
        // Camera-facing world labels (star names, markers) - overlay like TextPass.
        if(!runPass(RenderPass_WorldText, &SceneRenderer::WorldTextPass))
            Graphics::ClearWorldLabels();

        if(m_EnableUIPass)
            runPass(RenderPass_UI, &SceneRenderer::UIPass);

        runPass(RenderPass_Final, &SceneRenderer::FinalPass);
    }

    bool SceneRenderer::BeginPass(RenderPassID id)
    {
        RenderPassRecord& record = m_PassRecords[id];
        if(m_PassCutoff >= 0 && (i32)id > m_PassCutoff && id != RenderPass_UI && id != RenderPass_Final)
        {
            record.State = RenderPassState::CutOff;
            return false;
        }

        record.State     = RenderPassState::Ran;
        m_PassStartDraws = Engine::Get().Statistics().NumDrawCalls;
        s_PassStartTime  = Timer::Now();
        return true;
    }

    void SceneRenderer::EndPass(RenderPassID id)
    {
        RenderPassRecord& record = m_PassRecords[id];
        record.CpuMs             = Timer::Duration(s_PassStartTime, Timer::Now(), 1000.0f);
        record.DrawCalls         = Engine::Get().Statistics().NumDrawCalls - m_PassStartDraws;
    }

    void SceneRenderer::EnsureGBufferTargets()
    {
        if(m_GBufferAllocated)
            return;
        m_GBufferAllocated = true;
        m_NormalTexture->Resize(m_TargetWidth, m_TargetHeight);
        m_NormalResolveTexture->Resize(m_TargetWidth, m_TargetHeight);
        m_SSAOTexture->Resize(m_TargetWidth / 2, m_TargetHeight / 2);
        m_SSAOTexture1->Resize(m_TargetWidth / 2, m_TargetHeight / 2);
    }

    void SceneRenderer::EnsureBloomTargets()
    {
        if(m_BloomAllocated)
            return;
        m_BloomAllocated = true;
        m_BloomTexture->Resize(m_TargetWidth / 2, m_TargetHeight / 2);
        m_BloomTexture1->Resize(m_TargetWidth / 2, m_TargetHeight / 2);
        m_BloomTexture2->Resize(m_TargetWidth / 2, m_TargetHeight / 2);
    }

    void SceneRenderer::EnsureSMAATargets()
    {
        if(m_SMAAAllocated || !m_SMAAEdgesTexture || !m_SMAAWeightsTexture)
            return;
        m_SMAAAllocated = true;
        m_SMAAEdgesTexture->Resize(m_TargetWidth, m_TargetHeight);
        m_SMAAWeightsTexture->Resize(m_TargetWidth, m_TargetHeight);
    }

    void SceneRenderer::EnsureResolveTarget()
    {
        if(m_ResolveAllocated)
            return;
        m_ResolveAllocated = true;
        m_ResolveTexture->Resize(m_TargetWidth, m_TargetHeight);
    }

    void SceneRenderer::EnsureShadowTargets()
    {
        if(m_ShadowsAllocated)
            return;
        m_ShadowsAllocated = true;
        const uint32_t size = m_ShadowData.m_ShadowMapSize;
        m_ShadowData.m_ShadowTex->Resize(size, size, m_ShadowData.m_ShadowMapNum + m_ShadowData.m_SpotLayers);
        m_ShadowData.m_ShadowMapsInvalidated = true;
    }

    void SceneRenderer::EnsureTAATargets()
    {
        if(m_TAAAllocated)
            return;
        m_TAAAllocated = true;
        m_TAAHistory[0]->Resize(m_TargetWidth, m_TargetHeight);
        m_TAAHistory[1]->Resize(m_TargetWidth, m_TargetHeight);
        m_TAAHistoryValid = false;
    }

    bool SceneRenderer::TAAWanted() const
    {
        if(!m_CurrentScene || !m_TAAShader || !m_TAAShader->IsCompiled() || !m_TAADescriptorSet)
            return false;
        const auto& rs = m_CurrentScene->GetSettings().RenderSettings;
        // a learn mode cutoff before the resolve would show the raw jitter
        return rs.TAAEnabled && rs.Renderer3DEnabled && !m_DisablePostProcess && !m_TransparentBackground && m_MainTextureSamples == 1
            && m_Probe.Face < 0 && (m_PassCutoff < 0 || m_PassCutoff >= (i32)RenderPass_TAA);
    }

    void SceneRenderer::OnUpdate(const TimeStep& timeStep, Scene* scene)
    {
    }

    bool SceneRenderer::OnWindowResizeEvent(WindowResizeEvent& e)
    {
        LUMOS_PROFILE_FUNCTION();

        return false;
    }

    void SceneRenderer::OnEvent(Event& e)
    {
        LUMOS_PROFILE_FUNCTION();
        // EventDispatcher dispatcher(e);
        // dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(SceneRenderer::OnwindowResizeEvent));
    }

    const char* RenderModeToString(int mode)
    {
        switch(mode)
        {
        case 0:
            return "Lighting";
        case 1:
            return "Colour";
        case 2:
            return "Metallic";
        case 3:
            return "Roughness";
        case 4:
            return "AO";
        case 5:
            return "Emissive";
        case 6:
            return "Normal";
        case 7:
            return "Shadow Cascades";
        case 8:
            return "UV";
        case 9:
            return "GI";
        default:
            return "Lighting";
        }
    }

    void SceneRenderer::OnImGui()
    {
        LUMOS_PROFILE_FUNCTION();

        ImGui::TextUnformatted("Shadow Renderer");

        if(ImGui::TreeNode("Texture"))
        {
            static int index = 0;

            ImGui::SliderInt("Texture Array Index", &index, 0, m_ShadowData.m_ShadowTex->GetCount() - 1);

            bool flipImage = Renderer::GetGraphicsContext()->FlipImGUITexture();

            ImGuiUtilities::Image(m_ShadowData.m_ShadowTex, index, ImVec2(128, 128));

            if(ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGuiUtilities::Image(m_ShadowData.m_ShadowTex, index, ImVec2(256, 256));
                ImGui::EndTooltip();
            }

            ImGui::TreePop();
        }

        ImGui::DragFloat("Shadow Bias (texels)", &m_ShadowData.m_InitialBias, 0.01f, 0.0f, 8.0f, "%.2f");
        ImGui::DragFloat("Light Size", &m_ShadowData.m_LightSize, 0.00005f, 0.0f, 10.0f);
        ImGui::DragFloat("Max Shadow Distance", &m_ShadowData.m_MaxShadowDistance, 0.05f, 0.0f, 10000.0f);
        ImGui::DragFloat("Shadow Fade", &m_ShadowData.m_ShadowFade, 0.0005f, 0.0f, 500.0f);
        ImGui::DragFloat("Cascade Transition Fade", &m_ShadowData.m_CascadeFade, 0.0005f, 0.0f, 5.0f);

        ImGui::DragFloat("Cascade Split Lambda", &m_ShadowData.m_CascadeSplitLambda, 0.005f, 0.0f, 3.0f);

        ImGui::TextUnformatted("Forward Renderer");

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::Columns(2);
        ImGui::Separator();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Number Of Renderables");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        ImGui::Text("%5.2lu", m_ForwardData.m_CommandQueue.Size());
        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Render Mode");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::BeginMenu(RenderModeToString(m_ForwardData.m_RenderMode)))
        {
            const int numRenderModes = 10;

            for(int i = 0; i < numRenderModes; i++)
            {
                if(ImGui::MenuItem(RenderModeToString(i), "", m_ForwardData.m_RenderMode == i, true))
                {
                    m_ForwardData.m_RenderMode = i;
                }
            }
            ImGui::EndMenu();
        }
        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::Columns(1);
        ImGui::TextUnformatted("2D renderer");
        ImGui::Columns(2);

        ImGuiUtilities::Property("Number of draw calls", (int&)m_Renderer2DData.m_BatchDrawCallIndex, ImGuiUtilities::PropertyFlag::ReadOnly);
        ImGuiUtilities::Property("Max textures Per draw call", (int&)m_Renderer2DData.m_Limits.MaxTextures, 1, 16);
        ImGuiUtilities::Property("Exposure", m_Exposure);

        if(m_CurrentScene)
        {
            auto& rs = m_CurrentScene->GetSettings().RenderSettings;
            ImGuiUtilities::Property("Adaptive Exposure", rs.AdaptiveExposure);
            if(rs.AdaptiveExposure)
            {
                ImGuiUtilities::Property("Target Luminance", rs.AdaptiveTargetLuminance, 0.01f, 1.0f);
                ImGuiUtilities::Property("Min Log Luminance", rs.AdaptiveMinLogLum, -20.0f, 0.0f);
                ImGuiUtilities::Property("Max Log Luminance", rs.AdaptiveMaxLogLum, 0.0f, 20.0f);
                ImGuiUtilities::Property("Adapt Speed Up", rs.AdaptiveSpeedUp, 0.01f, 10.0f);
                ImGuiUtilities::Property("Adapt Speed Down", rs.AdaptiveSpeedDown, 0.01f, 10.0f);
            }
        }

        ImGui::Columns(1);
        ImGui::Separator();
        ImGui::PopStyleVar();
    }

    void SceneRenderer::OnNewScene(Scene* scene)
    {
        m_TAAHistoryValid     = false;
        m_ContactShadowsValid = false;
        if(m_GI)
            m_GI->Invalidate();
        m_ForwardData.m_EnvironmentMap = m_DefaultTextureCube;
        m_ForwardData.m_IrradianceMap  = m_DefaultTextureCube;

        Graphics::DescriptorDesc info    = {};
        info.shader                      = m_ForwardData.m_Shader.get();
        info.layoutIndex                 = 2;
        m_ForwardData.m_DescriptorSet[2] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(info));
        m_CubeMap                        = nullptr;

        Graphics::DescriptorDesc descriptorDesc = {};
        descriptorDesc.layoutIndex              = 0;
        descriptorDesc.shader                   = m_SkyboxShader.get();
        m_SkyboxDescriptorSet                   = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
    }

    float RoundUpToNearestMultipleOf5(float value)
    {
        // Divide the value by 5 and round it up to the nearest integer
        int roundedValue = static_cast<int>(std::ceil(value / 5.0f));

        // Multiply the rounded value by 5 to get the nearest multiple of 5
        float result = roundedValue * 5.0f;

        return result;
    }

    void SceneRenderer::PlanSpotShadows(Scene* scene, ShadowData::SpotCandidate* candidates, u32 count, GPULight* lights)
    {
        LUMOS_PROFILE_FUNCTION();
        const auto& renderSettings = scene->GetSettings().RenderSettings;
        auto& shadow               = m_ShadowData;

        // the most important first: near the camera, long reach
        for(u32 i = 1; i < count; i++)
            for(u32 j = i; j > 0 && candidates[j].Score > candidates[j - 1].Score; j--)
                Swap(candidates[j], candidates[j - 1]);
        count = Maths::Min(count, (u32)Maths::Clamp(renderSettings.MaxSpotShadows, 0, (int)SPOT_SHADOW_MAX));

        // biggest tiles first: with power-of-two sizes taken in that order, each tile starts on its own
        // size boundary along a Z-order curve, so they pack with no gaps
        for(u32 i = 1; i < count; i++)
            for(u32 j = i; j > 0 && candidates[j].Size > candidates[j - 1].Size; j--)
                Swap(candidates[j], candidates[j - 1]);

        const u32 mapSize  = shadow.m_ShadowMapSize;
        const u32 cell     = Maths::Min(256u, mapSize);
        const u32 cellsRow = mapSize / cell;
        u32 layer = 0, cursor = 0;

        for(u32 i = 0; i < count; i++)
        {
            const ShadowData::SpotCandidate& c = candidates[i];
            const u32 cells = c.Size / cell;
            const u32 area  = cells * cells;
            if(cursor + area > cellsRow * cellsRow)
            {
                layer++;
                cursor = 0;
            }
            if(layer >= SPOT_SHADOW_LAYERS)
                break;

            const Light& light = *c.Source;
            ShadowData::SpotShadow& spot = shadow.m_Spots[shadow.m_SpotCount];
            spot.LightID    = c.ID;
            spot.LightIndex = (i32)c.Index;
            spot.Layer      = layer;
            spot.X          = MortonCompact(cursor) * cell;
            spot.Y          = MortonCompact(cursor >> 1) * cell;
            spot.Size       = c.Size;
            spot.Score      = c.Score;
            spot.Bias       = light.ShadowBias;
            spot.NormalBias = light.ShadowNormalBias;
            spot.OnChange   = light.ShadowUpdate == (u8)LightShadowUpdate::OnChange;
            spot.Draw       = true;
            cursor += area;

            // the light shines down -Direction (Direction points back at it, like the sun's)
            const Vec3 position = Vec3(light.Position);
            Vec3 axis           = -Vec3(light.Direction);
            axis.Normalise();
            const Vec3 up         = Maths::Abs(axis.y) > 0.99f ? Vec3(1.0f, 0.0f, 0.0f) : Vec3(0.0f, 1.0f, 0.0f);
            const float halfAngle = Maths::Min(SpotShadowHalfAngle(light) * 1.05f + 0.02f, 1.48f); // a little over the cone, so PCF at its edge stays inside
            const float nearPlane = Maths::Clamp(light.ShadowNear, 0.001f, c.Far * 0.5f);
            const Mat4 view       = Mat4::LookAt(position, position + axis, up);
            const Mat4 proj       = Mat4::Perspective(nearPlane, c.Far, 1.0f, halfAngle * 2.0f * Maths::M_RADTODEG);
            spot.ProjView         = proj * view;
            spot.Frustum.Define(spot.ProjView);
            spot.TexelWorld = 2.0f * tanf(halfAngle) / (float)spot.Size;

            const float fadeStart = renderSettings.SpotShadowDistance * 0.8f;
            spot.Fade = 1.0f - Maths::Clamp((c.Distance - fadeStart) / Maths::Max(renderSettings.SpotShadowDistance - fadeStart, 0.001f), 0.0f, 1.0f);

            lights[c.Index].Direction.w = (float)shadow.m_SpotCount;
            shadow.m_SpotCommandQueue[shadow.m_SpotCount].Clear();
            shadow.m_SpotCount++;
        }

        // the atlas only grows: more layers on the array (its contents are lost, so everything is drawn again)
        const u32 layersNeeded = shadow.m_SpotCount > 0 ? shadow.m_Spots[shadow.m_SpotCount - 1].Layer + 1 : 0;
        if(layersNeeded > shadow.m_SpotLayers)
        {
            shadow.m_SpotLayers = layersNeeded;
            CascadeSchedule& schedule = CascadeScheduleFor(this);
            for(u32 i = 0; i < SHADOWMAP_MAX; i++)
                schedule.EmptyDrawn[i] = false;
            for(SpotTileCache& tile : schedule.Spots)
                tile.Valid = false;
            if(m_ShadowsAllocated)
                shadow.m_ShadowTex->Resize(mapSize, mapSize, shadow.m_ShadowMapNum + shadow.m_SpotLayers);
        }
    }

    void SceneRenderer::UpdateCascades(Scene* scene, Light* light)
    {
        LUMOS_PROFILE_FUNCTION();
        float cascadeSplits[SHADOWMAP_MAX];
        float cascadeRadius[SHADOWMAP_MAX];

        float nearClip  = m_Camera->GetNear();
        float farClip   = m_ShadowData.m_MaxShadowDistance * kShadowFarScale; // m_Camera->GetFar();
        float clipRange = farClip - nearClip;

        float minZ  = nearClip;
        float maxZ  = nearClip + clipRange;
        float range = maxZ - minZ;
        float ratio = maxZ / minZ;
        // Calculate split depths based on view camera frustum
        // Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
        for(uint32_t i = 0; i < m_ShadowData.m_ShadowMapNum; i++)
        {
            float p          = static_cast<float>(i + 1) / static_cast<float>(m_ShadowData.m_ShadowMapNum);
            float log        = minZ * std::pow(ratio, p);
            float uniform    = minZ + range * p;
            float d          = m_ShadowData.m_CascadeSplitLambda * (log - uniform) + uniform;
            cascadeSplits[i] = (d - nearClip) / clipRange;
        }

        // TODO: Fix need for this
        cascadeSplits[3]    = kLastCascadeSplit;
        float lastSplitDist = 0.0f;
        Mat4 CameraProj     = Mat4::Perspective(nearClip, farClip, m_Camera->GetAspectRatio(), m_Camera->GetFOV());

        const Mat4 invCam = (CameraProj * (m_CameraTransform->GetWorldMatrix().Inverse())).Inverse();

        for(uint32_t i = 0; i < m_ShadowData.m_ShadowMapNum; i++)
        {
            LUMOS_PROFILE_SCOPE("Create Cascade");
            float splitDist = cascadeSplits[i];

            Vec3 frustumCorners[8] = {
                Vec3(-1.0f, 1.0f, 0.0f),
                Vec3(1.0f, 1.0f, 0.0f),
                Vec3(1.0f, -1.0f, 0.0f),
                Vec3(-1.0f, -1.0f, 0.0f),
                Vec3(-1.0f, 1.0f, 1.0f),
                Vec3(1.0f, 1.0f, 1.0f),
                Vec3(1.0f, -1.0f, 1.0f),
                Vec3(-1.0f, -1.0f, 1.0f),
            };

            // Project frustum corners into world space
            for(uint32_t j = 0; j < 8; j++)
            {
                Vec4 invCorner    = invCam * Vec4(frustumCorners[j], 1.0f);
                frustumCorners[j] = (invCorner / invCorner.w);
            }

            for(uint32_t j = 0; j < 4; j++)
            {
                Vec3 dist             = frustumCorners[j + 4] - frustumCorners[j];
                frustumCorners[j + 4] = frustumCorners[j] + (dist * splitDist);
                frustumCorners[j]     = frustumCorners[j] + (dist * lastSplitDist);
            }

            lastSplitDist = cascadeSplits[i];

            // Get frustum center
            Vec3 frustumCenter = Vec3(0.0f);
            for(uint32_t j = 0; j < 8; j++)
            {
                frustumCenter += frustumCorners[j];
            }
            frustumCenter /= 8.0f;

            float radius = 0.0f;
            for(uint32_t j = 0; j < 8; j++)
            {
                float distance = Maths::Distance(frustumCorners[j], frustumCenter);
                radius         = Maths::Max(radius, distance);
            }

            // Temp work around to flickering when rotating camera
            // Sphere radius for lightOrthoMatrix should fix this
            // But radius changes as the camera is rotated which causes flickering
            // const float value = 16.0f;
            // radius = std::ceil(radius *value) / value;

            static const float roundTo[8] = { 5.0f, 5.0f, 20.0f, 200.0f, 400.0f, 400.0f, 400.0f, 400.0f };
            radius                        = RoundUpToNearestMultipleOf5(radius);

            cascadeRadius[i] = radius;

            Vec3 maxExtents = Vec3(radius);
            Vec3 minExtents = -maxExtents;

            Vec3 lightDir         = (-light->Direction).Normalised();
            //Mat4 lightOrthoMatrix = Mat4::Orthographic(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, -maxExtents.z - m_ShadowData.CascadeNearPlaneOffset, maxExtents.z + m_ShadowData.CascadeFarPlaneOffset);
            Mat4 lightOrthoMatrix = Mat4::Orthographic(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, m_ShadowData.CascadeNearPlaneOffset, maxExtents.z - minExtents.z + m_ShadowData.CascadeFarPlaneOffset);
            Mat4 LightViewMatrix  = Mat4::LookAt(frustumCenter - lightDir * -minExtents.z, frustumCenter, Vec3(0.0f, 1.0f, 0.0f));
            Mat4 shadowProj       = lightOrthoMatrix * LightViewMatrix;

            const bool StabilizeCascades = true;
            if(StabilizeCascades)
            {
                // Create the rounding matrix, by projecting the world-space origin and determining
                // the fractional offset in texel space
                Vec4 shadowOrigin = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
                shadowOrigin      = shadowProj * shadowOrigin;
                shadowOrigin *= (m_ShadowData.m_ShadowMapSize * 0.5f);

                Vec4 roundedOrigin = Vec4(Maths::Round(shadowOrigin.x), Maths::Round(shadowOrigin.y), Maths::Round(shadowOrigin.z), Maths::Round(shadowOrigin.w));
                Vec4 roundOffset   = roundedOrigin - shadowOrigin;
                roundOffset        = roundOffset * (2.0f / m_ShadowData.m_ShadowMapSize);
                roundOffset.z      = 0.0f;
                roundOffset.w      = 0.0f;

                lightOrthoMatrix.SetTranslation(lightOrthoMatrix.Translation() + Vec3(roundOffset.x, roundOffset.y, 0.0f));
            }
            // Store split distance and matrix in cascade
            m_ShadowData.m_SplitDepth[i]     = (m_Camera->GetNear() + splitDist * clipRange) * -1.0f;
            m_ShadowData.m_ShadowProjView[i] = lightOrthoMatrix * LightViewMatrix;

            if(i == 0)
                m_ShadowData.m_LightMatrix = LightViewMatrix.Inverse();
        }

        // a resized map starts blank: every cascade is cleared again
        CascadeSchedule& schedule = CascadeScheduleFor(this);
        if(schedule.MapSize != m_ShadowData.m_ShadowMapSize)
        {
            schedule.MapSize = m_ShadowData.m_ShadowMapSize;
            for(u32 i = 0; i < SHADOWMAP_MAX; i++)
                schedule.EmptyDrawn[i] = false;
        }
        for(u32 i = 0; i < SHADOWMAP_MAX; i++)
            schedule.Skip[i] = false;
    }

    void SceneRenderer::GenerateBRDFLUTPass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("BRDF Pass");

        if(!m_GenerateBRDFLUT)
            return;

        m_GenerateBRDFLUT = false;

        Graphics::PipelineDesc pipelineDesc {};
        pipelineDesc.shader = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("BRDFLUT")).As<Graphics::Shader>();

        pipelineDesc.polygonMode         = Graphics::PolygonMode::FILL;
        pipelineDesc.cullMode            = Graphics::CullMode::BACK;
        pipelineDesc.transparencyEnabled = false;
        pipelineDesc.colourTargets[0]    = m_ForwardData.m_BRDFLUT.get();
        pipelineDesc.DebugName           = "BRDF Generation";

        auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
        auto pipeline      = Graphics::Pipeline::Get(pipelineDesc);

        commandBuffer->BindPipeline(pipeline);
        Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);
        commandBuffer->UnBindPipeline();
    }

    void SceneRenderer::ShadowPass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("Shadow Pass");

        auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
        commandBuffer->UnBindPipeline();
        commandBuffer->EndCurrentRenderPass();

        m_ShadowData.m_DescriptorSet[0]->SetUniformBufferData(1, m_ShadowData.m_ShadowProjView);
        m_ShadowData.m_DescriptorSet[0]->Update();

        m_ShadowData.m_DescriptorSet[1]->SetUniformBufferData(1, m_ShadowData.m_ShadowProjView);
        m_ShadowData.m_DescriptorSet[1]->Update();

        Arena* frameArena                  = Application::Get().GetFrameArena();
        DescriptorSet** currentDescriptors = PushArrayNoZero(frameArena, DescriptorSet*, 4);

        // Instanced shadow pipelines only vary by cascade and alpha blending.
        SharedPtr<Graphics::Pipeline> instancedPipelines[SHADOWMAP_MAX * 2] = {};

        CascadeSchedule& schedule = CascadeScheduleFor(this);

        for(uint32_t i = 0; i < m_ShadowData.m_ShadowMapNum; ++i)
        {
            if(schedule.Skip[i])
                continue;
            // nothing to draw and already cleared: the slice is still right
            const bool empty = m_ShadowData.m_CascadeCommandQueue[i].Empty();
            if(empty && schedule.EmptyDrawn[i])
            {
                schedule.Skip[i] = true;
                continue;
            }
            schedule.EmptyDrawn[i] = empty;

            LUMOS_PROFILE_GPU("Shadow Cascade Pass");

            m_ShadowData.m_Layer = i;

            {
                // opens this layer's pass and clears inside it, so the draws below share the pass
                Graphics::PipelineDesc clearDesc;
                clearDesc.cullMode         = Graphics::CullMode::FRONT;
                SetShadowTarget(clearDesc, this, m_ShadowData.m_ShadowTex, i, m_ShadowData.m_ShadowMapSize);
                clearDesc.DebugName        = "Shadow Cascade Clear";
                clearDesc.clearTargets     = false;
                clearDesc.shader           = m_ShadowData.m_Shader;
                auto clearPipeline         = Graphics::Pipeline::Get(clearDesc);

                commandBuffer->BindPipeline(clearPipeline, i);
                commandBuffer->ClearDepthAttachment(1.0f);
            }

            for(auto& command : m_ShadowData.m_CascadeCommandQueue[i])
            {
                Material* material    = command.material ? command.material : m_ForwardData.m_DefaultMaterial;
                currentDescriptors[1] = material->GetDescriptorSet();
                bool alphaBlend       = material->GetFlag(Material::RenderFlags::ALPHABLEND);

                if(command.instanced && m_ShadowData.m_ShaderInstanced)
                {
                    SharedPtr<Graphics::Pipeline>& cachedInst = instancedPipelines[i * 2 + (alphaBlend ? 1 : 0)];
                    if(!cachedInst)
                    {
                        Graphics::PipelineDesc shadowInstPipelineDesc;
                        shadowInstPipelineDesc.cullMode                = Graphics::CullMode::BACK; // Match the non-instanced shadow pipeline.
                        SetShadowTarget(shadowInstPipelineDesc, this, m_ShadowData.m_ShadowTex, i, m_ShadowData.m_ShadowMapSize);
                        shadowInstPipelineDesc.depthBiasEnabled        = false;
                        shadowInstPipelineDesc.depthBiasConstantFactor = 0.0f;
                        shadowInstPipelineDesc.depthBiasSlopeFactor    = 0.0f;
                        shadowInstPipelineDesc.DebugName               = "Shadow Instanced";
                        shadowInstPipelineDesc.clearTargets            = false;
                        shadowInstPipelineDesc.transparencyEnabled     = alphaBlend;
                        shadowInstPipelineDesc.shader                  = alphaBlend ? m_ShadowData.m_ShaderInstancedAlpha : m_ShadowData.m_ShaderInstanced;

                        cachedInst = Graphics::Pipeline::Get(shadowInstPipelineDesc);
                    }
                    Graphics::Pipeline* pipeline = cachedInst.get();
                    commandBuffer->BindPipeline(pipeline, i);

                    currentDescriptors[0] = alphaBlend ? m_ShadowData.m_DescriptorSet[1].get() : m_ShadowData.m_DescriptorSet[0].get();
                    currentDescriptors[2] = m_ForwardData.m_DescriptorSet[2];
                    currentDescriptors[3] = m_ShadowData.m_InstanceDescriptorSet[Renderer::GetMainSwapChain()->GetCurrentBufferIndex()].get();

                    uint32_t layer      = static_cast<uint32_t>(m_ShadowData.m_Layer);
                    auto& pushConstants = pipeline->GetShader()->GetPushConstants();
                    memcpy(pushConstants[0].data, &layer, sizeof(uint32_t));
                    pipeline->GetShader()->BindPushConstants(commandBuffer, pipeline);

                    Renderer::BindDescriptorSets(pipeline, commandBuffer, 0, currentDescriptors, 4);
                    Renderer::DrawMeshInstanced(commandBuffer, pipeline, command.mesh, command.instanceCount, command.firstInstance);
                    m_Stats.NumShadowObjects += command.instanceCount;
                }
                else
                {
                    uint32_t layer      = static_cast<uint32_t>(m_ShadowData.m_Layer);
                    auto& pushConstants = command.pipeline->GetShader()->GetPushConstants();
                    memcpy(pushConstants[0].data + sizeof(Mat4), &layer, sizeof(uint32_t));
                    currentDescriptors[0] = alphaBlend ? m_ShadowData.m_DescriptorSet[1].get() : m_ShadowData.m_DescriptorSet[0].get();
                    currentDescriptors[2] = m_ForwardData.m_DescriptorSet[2];

                    if(command.animated)
                    {
                        currentDescriptors[3] = command.AnimatedDescriptorSet;
                    }

                    auto pipeline = command.pipeline;
                    commandBuffer->BindPipeline(pipeline, i);

                    Mesh* mesh     = command.mesh;
                    auto transform = m_ShadowData.m_ShadowProjView[m_ShadowData.m_Layer] * command.transform;
                    memcpy(pushConstants[0].data, &transform, sizeof(Mat4));

                    command.pipeline->GetShader()->BindPushConstants(commandBuffer, pipeline);
                    Renderer::BindDescriptorSets(pipeline, commandBuffer, 0, currentDescriptors, command.animated ? 4 : 3);
                    Renderer::DrawMesh(commandBuffer, pipeline, mesh);
                    m_Stats.NumShadowObjects++;
                }
            }
            commandBuffer->UnBindPipeline();
            commandBuffer->EndCurrentRenderPass();
        }

        bool spotLayerDrawn[SPOT_SHADOW_LAYERS] = {};
        SpotShadowPass(spotLayerDrawn);

        if(ShadowCopyPath())
        {
            LUMOS_PROFILE_GPU("Shadow Cascade Copy");
            for(uint32_t i = 0; i < m_ShadowData.m_ShadowMapNum; ++i)
            {
                if(schedule.Skip[i] || !schedule.Cascades[i])
                    continue;
                Renderer::GetRenderer()->CopyDepthToArraySlice(schedule.Cascades[i], reinterpret_cast<Texture*>(m_ShadowData.m_ShadowTex), i, commandBuffer);
            }
            for(u32 l = 0; l < SPOT_SHADOW_LAYERS; l++)
            {
                const u32 layer = m_ShadowData.m_ShadowMapNum + l;
                if(spotLayerDrawn[l] && schedule.Cascades[layer])
                    Renderer::GetRenderer()->CopyDepthToArraySlice(schedule.Cascades[layer], reinterpret_cast<Texture*>(m_ShadowData.m_ShadowTex), layer, commandBuffer);
            }
        }
    }

    void SceneRenderer::SpotShadowPass(bool* layerDrawn)
    {
        auto& shadow = m_ShadowData;
        if(shadow.m_SpotCount == 0)
            return;

        LUMOS_PROFILE_GPU("Spot Shadow Pass");
        auto commandBuffer                 = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
        DescriptorSet** currentDescriptors = PushArrayNoZero(Application::Get().GetFrameArena(), DescriptorSet*, 4);
        CascadeSchedule& schedule          = CascadeScheduleFor(this);
        const u32 mapSize                  = shadow.m_ShadowMapSize;

        // tiles are in layer order, so each layer's pass is opened once
        for(u32 i = 0; i < shadow.m_SpotCount; i++)
        {
            auto& spot = shadow.m_Spots[i];
            if(!spot.Draw)
                continue;

            const u32 layer         = shadow.m_ShadowMapNum + spot.Layer;
            layerDrawn[spot.Layer]  = true;

            {
                Graphics::PipelineDesc clearDesc;
                clearDesc.cullMode     = Graphics::CullMode::FRONT;
                SetShadowTarget(clearDesc, this, shadow.m_ShadowTex, layer, mapSize);
                clearDesc.DebugName    = "Spot Shadow Clear";
                clearDesc.clearTargets = false;
                clearDesc.shader       = shadow.m_Shader;
                commandBuffer->BindPipeline(Graphics::Pipeline::Get(clearDesc), layer);
                commandBuffer->SetViewportRect(spot.X, spot.Y, spot.Size, spot.Size);
                commandBuffer->ClearDepthAttachmentRect(1.0f, spot.X, spot.Y, spot.Size, spot.Size);
            }

            for(auto& command : shadow.m_SpotCommandQueue[i])
            {
                Material* material    = command.material ? command.material : m_ForwardData.m_DefaultMaterial;
                const bool alphaBlend = material->GetFlag(Material::RenderFlags::ALPHABLEND);
                currentDescriptors[0] = alphaBlend ? shadow.m_DescriptorSet[1].get() : shadow.m_DescriptorSet[0].get();
                currentDescriptors[1] = material->GetDescriptorSet();
                currentDescriptors[2] = m_ForwardData.m_DescriptorSet[2];
                if(command.animated)
                    currentDescriptors[3] = command.AnimatedDescriptorSet;

                auto pipeline = command.pipeline;
                commandBuffer->BindPipeline(pipeline, layer);
                // binding can open the layer's pass, which resets the viewport to the whole layer
                commandBuffer->SetViewportRect(spot.X, spot.Y, spot.Size, spot.Size);

                auto& pushConstants = pipeline->GetShader()->GetPushConstants();
                const Mat4 transform = spot.ProjView * command.transform;
                memcpy(pushConstants[0].data, &transform, sizeof(Mat4));
                memcpy(pushConstants[0].data + sizeof(Mat4), &layer, sizeof(uint32_t));
                pipeline->GetShader()->BindPushConstants(commandBuffer, pipeline);

                Renderer::BindDescriptorSets(pipeline, commandBuffer, 0, currentDescriptors, command.animated ? 4 : 3);
                Renderer::DrawMesh(commandBuffer, pipeline, command.mesh);
                m_Stats.NumShadowObjects++;
            }
        }
        commandBuffer->UnBindPipeline();
        commandBuffer->EndCurrentRenderPass();

        // remember what each tile now holds
        for(u32 i = 0; i < SPOT_SHADOW_MAX; i++)
            schedule.Spots[i].Valid = false;
        for(u32 i = 0; i < shadow.m_SpotCount; i++)
        {
            const auto& spot  = shadow.m_Spots[i];
            SpotTileCache& tile = schedule.Spots[i];
            tile.LightID = spot.LightID;
            tile.Hash    = spot.Hash;
            tile.Layer   = spot.Layer;
            tile.X       = spot.X;
            tile.Y       = spot.Y;
            tile.Size    = spot.Size;
            tile.Valid   = true;
        }
    }

    void SceneRenderer::DepthPrePass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("Depth Pre Pass");

        Graphics::CommandBuffer* commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
        Graphics::PipelineDesc pipelineDesc {};
        pipelineDesc.polygonMode      = Graphics::PolygonMode::FILL;
        pipelineDesc.blendMode        = BlendMode::SrcAlphaOneMinusSrcAlpha;
        pipelineDesc.clearTargets     = false;
        pipelineDesc.swapchainTarget  = false;
        pipelineDesc.cullMode         = Graphics::CullMode::BACK;
        pipelineDesc.depthTarget      = m_ForwardData.m_DepthTexture;
        pipelineDesc.colourTargets[0] = m_NormalTexture;
        pipelineDesc.DebugName        = "Depth Prepass";
        pipelineDesc.samples          = m_MainTextureSamples;
        // Resolve the packed G-buffer so SSR has a single-sample copy to read.
        if(m_MainTextureSamples > 1)
            pipelineDesc.resolveTexture = m_NormalResolveTexture;

        DescriptorSet* sets[4];
        sets[0] = m_ForwardData.m_DescriptorSet[0].get();
        sets[2] = m_ForwardData.m_DescriptorSet[2].get();

        // Only alpha / animated / instanced vary the pipeline, so memoise (Pipeline::Get hashes the whole desc)
        SharedPtr<Graphics::Pipeline> cache[8] = {};
        Graphics::Pipeline* lastPipeline       = nullptr;
        const u32 frameIdx                     = Renderer::GetMainSwapChain()->GetCurrentBufferIndex();
        const bool canInstance                 = m_DepthPrePassInstancedShader && m_ForwardData.m_InstanceDescriptorSet.Size() > frameIdx;

        for(auto& command : m_ForwardData.m_CommandQueue)
        {
            Material* material = command.material ? command.material : m_ForwardData.m_DefaultMaterial;
            if(!material->GetFlag(Material::RenderFlags::DEPTHTEST))
                continue;

            const bool alphaBlend = material->GetFlag(Material::RenderFlags::ALPHABLEND);
            const bool instanced  = command.instanced && canInstance;
            const bool animated   = command.animated;
            // instanced commands are never animated or blended (see batching)
            SharedPtr<Graphics::Pipeline>& cached = cache[(alphaBlend ? 4 : 0) | (animated ? 2 : 0) | (instanced ? 1 : 0)];
            if(!cached)
            {
                pipelineDesc.transparencyEnabled = alphaBlend;
                pipelineDesc.shader              = instanced ? m_DepthPrePassInstancedShader
                    : animated                               ? (alphaBlend ? m_DepthPrePassAlphaAnimShader : m_DepthPrePassAnimShader)
                                                             : (alphaBlend ? m_DepthPrePassAlphaShader : m_DepthPrePassShader);
                cached                           = Graphics::Pipeline::Get(pipelineDesc);
            }
            Graphics::Pipeline* pipeline = cached.get();
            if(pipeline != lastPipeline)
            {
                commandBuffer->BindPipeline(pipeline);
                lastPipeline = pipeline;
            }

            sets[1] = material->GetDescriptorSet();
            if(instanced)
                sets[3] = m_ForwardData.m_InstanceDescriptorSet[frameIdx].get();
            else if(animated)
                sets[3] = command.AnimatedDescriptorSet;

            Renderer::BindDescriptorSets(pipeline, commandBuffer, 0, sets, (instanced || animated) ? 4 : 3);

            if(instanced)
            {
                Renderer::DrawMeshInstanced(commandBuffer, pipeline, command.mesh, command.instanceCount, command.firstInstance);
            }
            else
            {
                auto& pushConstants = m_DepthPrePassShader->GetPushConstants()[0];
                pushConstants.SetData((void*)&command.transform);
                m_DepthPrePassShader->BindPushConstants(commandBuffer, pipeline);
                Renderer::DrawMesh(commandBuffer, pipeline, command.mesh);
            }
        }
    }

    // Swaps the forward shaders when the quality level changes; pipelines are keyed by shader so they follow
    void SceneRenderer::ApplyForwardQuality()
    {
        const bool low = Application::Get().GetQualitySettings().LowForwardShader;
        auto pick      = [low](const SharedPtr<Graphics::Shader>* pair) -> const SharedPtr<Graphics::Shader>&
        { return (low && pair[1] && pair[1]->IsCompiled()) ? pair[1] : pair[0]; };

        if(m_ForwardShaders[0])
            m_ForwardData.m_Shader = pick(m_ForwardShaders);
        if(m_InstancedShaders[0])
            m_ForwardData.m_InstancedShader = pick(m_InstancedShaders);
    }

    void SceneRenderer::InitOcclusion()
    {
        if(!m_SupportCompute)
            return;

        auto& oc = m_Occlusion;
        oc.Shader = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("OcclusionGrid")).As<Graphics::Shader>();
        if(!oc.Shader || !oc.Shader->IsCompiled())
        {
            oc.Shader = nullptr;
            LWARN("Occlusion grid shader not available - occlusion culling off");
            return;
        }

        const u32 slots = Renderer::GetMainSwapChain()->GetSwapChainBufferCount();
        for(u32 i = 0; i < slots; i++)
        {
            oc.Buffers.PushBack(StorageBuffer::Create(OcclusionData::GridX * OcclusionData::GridY * sizeof(float), nullptr, true));
            Graphics::DescriptorDesc desc {};
            desc.layoutIndex = 0;
            desc.shader      = oc.Shader.get();
            oc.Sets.PushBack(SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(desc)));
            oc.SlotViewProj.PushBack(Mat4());
            oc.SlotValid.PushBack(0);
        }

        u32 w = OcclusionData::GridX, h = OcclusionData::GridY, offset = 0;
        while(oc.LevelCount < OcclusionData::MaxLevels)
        {
            oc.Levels[oc.LevelCount] = oc.Storage + offset;
            oc.LevelW[oc.LevelCount] = w;
            oc.LevelH[oc.LevelCount] = h;
            oc.LevelCount++;
            offset += w * h;
            if(w == 1 && h == 1)
                break;
            w = (w + 1) / 2;
            h = (h + 1) / 2;
        }
    }

    // Runs after the depth pre-pass (or the forward pass without one): farthest depth per grid cell into this frame slot's buffer.
    void SceneRenderer::OcclusionGridPass()
    {
        auto& oc = m_Occlusion;
        auto& rs = m_CurrentScene->GetSettings().RenderSettings;
        if(!oc.Shader || !rs.OcclusionCulling || !rs.Renderer3DEnabled || !m_Has3D || m_MainTextureSamples > 1 || !m_Camera || m_Camera->IsOrthographic() || m_Probe.Face >= 0)
            return;

        const u32 slot = Renderer::GetMainSwapChain()->GetCurrentBufferIndex();
        if(slot >= oc.Buffers.Size())
            return;

        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("Occlusion Grid Pass");

        auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
        commandBuffer->UnBindPipeline();

        auto* depth = reinterpret_cast<Texture*>(m_ForwardData.m_DepthTexture);
        oc.Sets[slot]->SetTexture(0, depth);
        oc.Sets[slot]->SetStorageBuffer(1, oc.Buffers[slot]);
        oc.Sets[slot]->Update();
        oc.Sets[slot]->TransitionImages(commandBuffer);

        Graphics::PipelineDesc pipelineDesc {};
        pipelineDesc.shader    = oc.Shader;
        pipelineDesc.DebugName = "OcclusionGrid";
        auto pipeline          = Graphics::Pipeline::Get(pipelineDesc);
        commandBuffer->BindPipeline(pipeline);

        struct PC
        {
            u32 TexWidth, TexHeight, GridX, GridY;
        } pc = { depth->GetWidth(), depth->GetHeight(), OcclusionData::GridX, OcclusionData::GridY };

        auto& pushConstants = oc.Shader->GetPushConstants()[0];
        pushConstants.SetData(&pc);
        oc.Shader->BindPushConstants(commandBuffer, pipeline.get());

        auto set = oc.Sets[slot].get();
        Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
        Renderer::GetRenderer()->Dispatch(commandBuffer, OcclusionData::GridX, OcclusionData::GridY, 1);

        // host reads it after this slot's fence wait, two frames on
        oc.SlotViewProj[slot] = m_Camera->GetProjectionMatrix() * m_CameraTransform->GetWorldMatrix().Inverse();
        oc.SlotValid[slot]    = 1;
    }

    // Start of the frame: pull the grid this slot's frame wrote two frames ago.
    void SceneRenderer::UpdateOcclusion(const Mat4& currentViewProj, bool enabled)
    {
        auto& oc  = m_Occlusion;
        oc.Active = false;
        if(!oc.Shader)
            return;

        // another world: the old depth says nothing about this one
        if(oc.LastScene != m_CurrentScene)
        {
            oc.LastScene = m_CurrentScene;
            for(u32 i = 0; i < oc.SlotValid.Size(); i++)
                oc.SlotValid[i] = 0;
        }

        const u32 slot = Renderer::GetMainSwapChain()->GetCurrentBufferIndex();
        if(!enabled || slot >= oc.Buffers.Size() || !oc.SlotValid[slot])
            return;
        oc.SlotValid[slot] = 0;

        const float* src = oc.Buffers[slot]->GetPointer<float>();
        memcpy(oc.Levels[0], src, OcclusionData::GridX * OcclusionData::GridY * sizeof(float));
        oc.Buffers[slot]->Unmap();

        for(u32 l = 1; l < oc.LevelCount; l++)
        {
            const u32 pw = oc.LevelW[l - 1], ph = oc.LevelH[l - 1];
            const u32 w = oc.LevelW[l], h = oc.LevelH[l];
            for(u32 y = 0; y < h; y++)
            {
                const u32 y0 = y * 2, y1 = Maths::Min(y0 + 1, ph - 1);
                for(u32 x = 0; x < w; x++)
                {
                    const u32 x0 = x * 2, x1 = Maths::Min(x0 + 1, pw - 1);
                    const float* p = oc.Levels[l - 1];
                    oc.Levels[l][y * w + x] = Maths::Max(Maths::Max(p[y0 * pw + x0], p[y0 * pw + x1]), Maths::Max(p[y1 * pw + x0], p[y1 * pw + x1]));
                }
            }
        }

        oc.TestViewProj = oc.SlotViewProj[slot];
        oc.Active       = true;
    }

    // True when the box is fully behind what the old depth grid saw. Anything unsure draws.
    bool SceneRenderer::IsOccluded(const Maths::BoundingBox& box) const
    {
        const auto& oc = m_Occlusion;
        const Vec3 mn  = box.m_Min;
        const Vec3 mx  = box.m_Max;

        float x0 = 1e30f, y0 = 1e30f, x1 = -1e30f, y1 = -1e30f, nearest = 1e30f;
        for(int i = 0; i < 8; i++)
        {
            const Vec4 c = oc.TestViewProj * Vec4((i & 1) ? mx.x : mn.x, (i & 2) ? mx.y : mn.y, (i & 4) ? mx.z : mn.z, 1.0f);
            if(c.w <= 0.001f) // crosses the eye plane
                return false;
            const float iw = 1.0f / c.w;
            const float x = c.x * iw, y = c.y * iw, z = c.z * iw;
            x0      = Maths::Min(x0, x);
            x1      = Maths::Max(x1, x);
            y0      = Maths::Min(y0, y);
            y1      = Maths::Max(y1, y);
            nearest = Maths::Min(nearest, z);
        }

        // partly off the old screen: the hidden part was never seen
        if(x0 < -1.0f || x1 > 1.0f || y0 < -1.0f || y1 > 1.0f || nearest < 0.0f)
            return false;

        // a couple of cells of slack so a camera that moved since still sees what it should
        const float gx = (float)OcclusionData::GridX, gy = (float)OcclusionData::GridY;
        const int slack = 2;
        int cx0 = Maths::Max(0, (int)((x0 * 0.5f + 0.5f) * gx) - slack);
        int cx1 = Maths::Min((int)OcclusionData::GridX - 1, (int)((x1 * 0.5f + 0.5f) * gx) + slack);
        int cy0 = Maths::Max(0, (int)((y0 * 0.5f + 0.5f) * gy) - slack);
        int cy1 = Maths::Min((int)OcclusionData::GridY - 1, (int)((y1 * 0.5f + 0.5f) * gy) + slack);

        // level where the rect spans at most 2 cells a side
        const int span = Maths::Max(cx1 - cx0, cy1 - cy0) + 1;
        u32 level      = 0;
        while((1 << level) < span && level + 1 < oc.LevelCount)
            level++;

        float farthest = 0.0f;
        for(int y = cy0 >> level; y <= (cy1 >> level); y++)
            for(int x = cx0 >> level; x <= (cx1 >> level); x++)
                farthest = Maths::Max(farthest, oc.Levels[level][y * oc.LevelW[level] + x]);

        // depth crowds towards 1, so compare how far each is from 1 (~1/distance) with a margin
        return (1.0f - nearest) < (1.0f - farthest) * 0.97f;
    }

    void SceneRenderer::SSAOPass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("SSAO Pass");

        if(m_MainTextureSamples > 1)
        {
            // LWARN("SSAOPass pass currently not working with msaa");
            return;
        }

        if(!m_Camera || !m_SSAOShader || !m_SSAOShader->IsCompiled())
            return;

        Graphics::CommandBuffer* commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
        Graphics::PipelineDesc pipelineDesc {};
        pipelineDesc.shader           = m_SSAOShader;
        pipelineDesc.colourTargets[0] = m_SSAOTexture;
        pipelineDesc.clearTargets     = true;
        pipelineDesc.DebugName        = "SSAO";
        pipelineDesc.samples          = m_MainTextureSamples;
        if(m_MainTextureSamples > 1)
            pipelineDesc.resolveTexture = m_ResolveTexture;

        auto projection = m_Camera->GetGPUProjectionMatrix();
        auto invProj    = m_Camera->GetGPUProjectionMatrix().Inverse();
        auto view       = m_CameraTransform->GetWorldMatrix().Inverse();

        float nearC = m_Camera->GetNear();
        float farC  = m_Camera->GetFar();

        static bool init = false;
        struct SSAOUniformBufferData
        {
            Mat4 projection;
            Mat4 invProj;
            Mat4 view;
            Vec4 samples[64];
            float ssaoRadius;
            float Near;
            float Far;
            float strength;
            int SampleCount; // quality-driven kernel size
            int _pad0;
            int _pad1;
            int _pad2;
        };
        static SSAOUniformBufferData data;

        if(!init)
        {
            for(uint32_t i = 0; i < 64; ++i)
            {
                Vec3 sample(Random32::Rand(-0.9f, 0.9f), Random32::Rand(-0.9f, 0.9f), Random32::Rand(0.0f, 1.0f));
                sample.Normalise();                   // Snap to surface of hemisphere
                sample *= Random32::Rand(0.0f, 1.0f); // Space out linearly
                float scale     = (float)i / (float)64;
                scale           = Maths::Lerp(0.1f, 1.0f, scale * scale); // Bring distribution of samples closer to origin
                data.samples[i] = Vec4(sample * scale, 0.0f);
            }
            init = true;
        }

        Scene::SceneRenderSettings& renderSettings = m_CurrentScene->GetSettings().RenderSettings;
        data.invProj                               = invProj;
        data.projection                            = projection;
        data.view                                  = view;
        data.ssaoRadius                            = renderSettings.SSAOSampleRadius;
        data.Near                                  = nearC;
        data.Far                                   = farC;
        data.strength                              = renderSettings.SSAOStrength;
        data.SampleCount                           = Maths::Clamp(Application::Get().GetQualitySettings().SSAOSampleCount(), 1, 64);
        data._pad0 = data._pad1 = data._pad2 = 0;

        m_SSAOPassDescriptorSet->SetUniformBufferData(0, &data);
        m_SSAOPassDescriptorSet->SetTexture(3, m_ForwardData.m_DepthTexture);
        m_SSAOPassDescriptorSet->SetTexture(2, m_NoiseTexture);
        m_SSAOPassDescriptorSet->SetTexture(1, m_NormalTexture);
        m_SSAOPassDescriptorSet->Update();

        auto pipeline = Graphics::Pipeline::Get(pipelineDesc);
        commandBuffer->BindPipeline(pipeline);

        auto set = m_SSAOPassDescriptorSet.get();
        Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
        Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);

        m_SSAOValid = true;
    }

    void SceneRenderer::SSAOBlurPass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("SSAO Blur Pass");

        if(!m_Camera || !m_SSAOBlurShader || !m_SSAOBlurShader->IsCompiled())
            return;

        Graphics::CommandBuffer* commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
        Graphics::PipelineDesc pipelineDesc {};
        pipelineDesc.shader           = m_SSAOBlurShader;
        pipelineDesc.colourTargets[0] = m_SSAOTexture1;
        pipelineDesc.clearTargets     = true;
        pipelineDesc.DebugName        = "SSAO Blur";
        pipelineDesc.samples          = m_MainTextureSamples;
        if(m_MainTextureSamples > 1)
            pipelineDesc.resolveTexture = m_ResolveTexture;

        Scene::SceneRenderSettings& renderSettings = m_CurrentScene->GetSettings().RenderSettings;
        auto view                                  = m_CameraTransform->GetWorldMatrix().Inverse();

        struct SSAOUniformBufferData
        {
            Mat4 view;
            float texelOffset[2];
            int blurRadius;
            int pad;
            float Near;
            float Far;
        };
        static SSAOUniformBufferData data;
        data.view           = view;
        data.texelOffset[0] = 1.0f / m_SSAOTexture->GetWidth();
        data.texelOffset[1] = 0.0f;

        // taps one texel apart: at least 5 of them to average out the 4x4 noise tile
        data.blurRadius = Maths::Clamp(renderSettings.SSAOBlurRadius, 2, 4);
        data.Near       = m_Camera->GetNear();
        data.Far        = m_Camera->GetFar();

        m_SSAOBlurPassDescriptorSet->SetUniformBufferData(0, &data);
        m_SSAOBlurPassDescriptorSet->SetTexture(3, m_ForwardData.m_DepthTexture);
        m_SSAOBlurPassDescriptorSet->SetTexture(1, m_SSAOTexture);
        m_SSAOBlurPassDescriptorSet->SetTexture(2, m_NormalTexture);
        m_SSAOBlurPassDescriptorSet->TransitionImages(commandBuffer);
        m_SSAOBlurPassDescriptorSet->Update();

        data.texelOffset[1] = 1.0f / m_SSAOTexture->GetHeight();
        data.texelOffset[0] = 0.0f;

        m_SSAOBlurPassDescriptorSet2->SetUniformBufferData(0, &data);
        m_SSAOBlurPassDescriptorSet2->SetTexture(3, m_ForwardData.m_DepthTexture);
        m_SSAOBlurPassDescriptorSet2->SetTexture(1, m_SSAOTexture1);
        m_SSAOBlurPassDescriptorSet2->SetTexture(2, m_NormalTexture);

        auto pipeline = Graphics::Pipeline::Get(pipelineDesc);
        commandBuffer->BindPipeline(pipeline);

        auto set = m_SSAOBlurPassDescriptorSet.get();
        Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
        Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);

        m_SSAOBlurPassDescriptorSet2->TransitionImages(commandBuffer);
        m_SSAOBlurPassDescriptorSet2->Update();

        pipelineDesc.colourTargets[0] = m_SSAOTexture;
        pipeline                      = Graphics::Pipeline::Get(pipelineDesc);
        commandBuffer->BindPipeline(pipeline);

        set = m_SSAOBlurPassDescriptorSet2.get();
        Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
        Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);
    }

    void SceneRenderer::LightCullingPass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("Light Culling Pass");

        if(!m_LightCullingShader || !m_LightCullingDescriptorSet || !m_Camera || !m_CameraTransform)
            return;

        // only directional lights: every tile sees all of them, so the lists are written once
        // instead of a compute pass reading every depth pixel each frame
        if(!m_HasLocalLights)
        {
            const uint32_t count = Maths::Min(m_NumLights, (uint32_t)MAX_LIGHTS_PER_TILE);
            if(m_StaticGridLights != count)
            {
                m_StaticGridLights = count;
                m_StaticGridWrites = 3;
            }
            if(m_StaticGridWrites > 0)
            {
                m_StaticGridWrites--;
                const uint32_t tileCount = m_TileCountX * m_TileCountY;
                ArenaTemp scratch        = ScratchBegin(0, 0);
                uint32_t* grid           = PushArrayNoZero(scratch.arena, uint32_t, tileCount * 2);
                for(uint32_t t = 0; t < tileCount; t++)
                {
                    grid[t * 2]     = 0;
                    grid[t * 2 + 1] = count;
                }
                m_LightGridSSBO->SetData(tileCount * sizeof(uint32_t) * 2, grid);
                uint32_t indices[MAX_LIGHTS_PER_TILE];
                for(uint32_t i = 0; i < count; i++)
                    indices[i] = i;
                if(count)
                    m_LightIndexSSBO->SetData(count * sizeof(uint32_t), indices);
                ScratchEnd(scratch);
            }
            return;
        }
        m_StaticGridLights = ~0u;

        Graphics::CommandBuffer* commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();

        // Reset global index counter
        uint32_t zero = 0;
        m_GlobalIndexCountSSBO->SetData(sizeof(uint32_t), &zero);

        // Bind depth texture and SSBOs to culling descriptor set
        m_LightCullingDescriptorSet->SetTexture(0, reinterpret_cast<Texture*>(m_ForwardData.m_DepthTexture));
        m_LightCullingDescriptorSet->SetStorageBuffer(1, m_LightSSBO);
        m_LightCullingDescriptorSet->SetStorageBuffer(2, m_LightGridSSBO);
        m_LightCullingDescriptorSet->SetStorageBuffer(3, m_LightIndexSSBO);
        m_LightCullingDescriptorSet->SetStorageBuffer(4, m_GlobalIndexCountSSBO);
        m_LightCullingDescriptorSet->Update();

        // Create compute pipeline
        Graphics::PipelineDesc pipelineDesc {};
        pipelineDesc.shader   = m_LightCullingShader;
        pipelineDesc.DebugName = "LightCulling";
        auto pipeline = Graphics::Pipeline::Get(pipelineDesc);
        commandBuffer->BindPipeline(pipeline);

        // Push constants
        struct LightCullingPushConstants
        {
            Mat4 invViewProj;
            uint32_t screenWidth;
            uint32_t screenHeight;
            uint32_t tileCountX;
            uint32_t tileCountY;
            uint32_t lightCount;
            float hasDepth;
            float pad0;
            float rangeMode;
        };

        LightCullingPushConstants pushData;
        pushData.invViewProj   = (m_Camera->GetGPUProjectionMatrix() * m_CameraTransform->GetWorldMatrix().Inverse()).Inverse();
        pushData.screenWidth   = m_MainTexture->GetWidth();
        pushData.screenHeight  = m_MainTexture->GetHeight();
        pushData.tileCountX    = m_TileCountX;
        pushData.tileCountY    = m_TileCountY;
        pushData.lightCount    = m_NumLights;
        // without a pre-pass the depth buffer still holds last frame: cull on the full range
        pushData.hasDepth      = s_CullDepthValid ? 1.0f : 0.0f;
        pushData.pad0          = 0.0f;
        pushData.rangeMode     = m_CurrentScene->GetSettings().RenderSettings.LightRange ? 1.0f : 0.0f;

        auto& pushConstants = m_LightCullingShader->GetPushConstants()[0];
        pushConstants.SetData(&pushData);
        m_LightCullingShader->BindPushConstants(commandBuffer, pipeline.get());

        auto set = m_LightCullingDescriptorSet.get();
        Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
        Renderer::GetRenderer()->Dispatch(commandBuffer, m_TileCountX, m_TileCountY, 1);

        // Memory barrier: compute write -> fragment read
        commandBuffer->BufferMemoryBarrier(
            CommandBuffer::PipelineStage::COMPUTE_SHADER,
            CommandBuffer::PipelineStage::FRAGMENT_SHADER);
    }

    void SceneRenderer::ForwardPass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("Forward Pass");

        {
            Graphics::CommandBuffer* cb = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
            Graphics::PipelineDesc clearDesc {};
            clearDesc.shader           = m_ForwardData.m_Shader;
            clearDesc.colourTargets[0] = m_MainTexture;
            clearDesc.depthTarget      = reinterpret_cast<Texture*>(m_ForwardData.m_DepthTexture);
            clearDesc.clearTargets     = true;
            const Vec4 clear           = m_TransparentBackground ? Vec4(0.0f) : Vec4(m_ClearColour.x, m_ClearColour.y, m_ClearColour.z, 1.0f);
            clearDesc.clearColour[0]   = clear.x;
            clearDesc.clearColour[1]   = clear.y;
            clearDesc.clearColour[2]   = clear.z;
            clearDesc.clearColour[3]   = clear.w;
            clearDesc.samples          = m_MainTextureSamples;
            if(m_MainTextureSamples > 1)
                clearDesc.resolveTexture = m_ResolveTexture;
            auto clearPipeline = Graphics::Pipeline::Get(clearDesc);
            cb->BindPipeline(clearPipeline);
            cb->EndCurrentRenderPass();
        }

        if(m_ForwardData.m_CommandQueue.Empty())
            return;

        Graphics::CommandBuffer* commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
        m_ForwardData.m_DescriptorSet[2]->TransitionImages(commandBuffer);

        Arena* frameArena                  = Application::Get().GetFrameArena();
        DescriptorSet** currentDescriptors = PushArrayNoZero(frameArena, DescriptorSet*, 4);

        for(auto& command : m_ForwardData.m_CommandQueue)
        {
            m_Stats.NumRenderedObjects++;

            Mesh* mesh         = command.mesh;
            Material* material = command.material ? command.material : m_ForwardData.m_DefaultMaterial;

            if(command.instanced && m_ForwardData.m_InstancedShader)
            {
                // Instanced draw: build pipeline with instanced shader
                Graphics::PipelineDesc instancedPipelineDesc {};
                instancedPipelineDesc.shader              = m_ForwardData.m_InstancedShader;
                instancedPipelineDesc.polygonMode         = Graphics::PolygonMode::FILL;
                instancedPipelineDesc.blendMode           = BlendMode::SrcAlphaOneMinusSrcAlpha;
                instancedPipelineDesc.clearTargets        = false;
                instancedPipelineDesc.swapchainTarget     = false;
                instancedPipelineDesc.colourTargets[0]    = m_MainTexture;
                instancedPipelineDesc.cullMode            = material->GetFlag(Material::RenderFlags::TWOSIDED) ? Graphics::CullMode::NONE : Graphics::CullMode::BACK;
                instancedPipelineDesc.transparencyEnabled = false;
                instancedPipelineDesc.samples             = m_MainTextureSamples;
                if(m_MainTextureSamples > 1)
                    instancedPipelineDesc.resolveTexture = m_ResolveTexture;
                if(m_ForwardData.m_DepthTest && material->GetFlag(Material::RenderFlags::DEPTHTEST))
                    instancedPipelineDesc.depthTarget = m_ForwardData.m_DepthTexture;
                instancedPipelineDesc.DebugName = "Forward PBR Instanced";

                auto pipeline = Graphics::Pipeline::Get(instancedPipelineDesc);
                commandBuffer->BindPipeline(pipeline);

                currentDescriptors[0] = m_ForwardData.m_DescriptorSet[0].get();
                currentDescriptors[1] = material->GetDescriptorSet();
                currentDescriptors[2] = m_ForwardData.m_DescriptorSet[2].get();
                currentDescriptors[3] = m_ForwardData.m_InstanceDescriptorSet[Renderer::GetMainSwapChain()->GetCurrentBufferIndex()].get();

                Renderer::BindDescriptorSets(pipeline, commandBuffer, 0, currentDescriptors, 4);
                Renderer::DrawMeshInstanced(commandBuffer, pipeline, mesh, command.instanceCount, command.firstInstance);
                m_Stats.NumRenderedObjects += command.instanceCount - 1;
            }
            else
            {
                // Non-instanced draw (original path)
                auto& worldTransform = command.transform;
                auto pipeline        = command.pipeline;
                commandBuffer->BindPipeline(pipeline);

                currentDescriptors[0] = m_ForwardData.m_DescriptorSet[0].get();
                currentDescriptors[1] = material->GetDescriptorSet();
                currentDescriptors[2] = m_ForwardData.m_DescriptorSet[2].get();

                if(command.animated)
                    currentDescriptors[3] = command.AnimatedDescriptorSet ? command.AnimatedDescriptorSet : m_ForwardData.m_DescriptorSet[3].get();

                auto& pushConstants = m_ForwardData.m_Shader->GetPushConstants()[0];
                pushConstants.SetData((void*)&worldTransform);

                m_ForwardData.m_Shader->BindPushConstants(commandBuffer, pipeline);
                Renderer::BindDescriptorSets(pipeline, commandBuffer, 0, currentDescriptors, command.animated ? 4 : 3);
                Renderer::DrawMesh(commandBuffer, pipeline, mesh);
            }
        }
    }

    // Draws a whole sky into the picture not on show. Runs outside the main pass.
    void SceneRenderer::CosmoBake()
    {
        LUMOS_PROFILE_FUNCTION();

        auto& cosmo = Graphics::CosmoBackdrop::Get();
        if(!cosmo.Enabled || !m_CosmoBakeShader || !m_CosmoBakeShader->IsCompiled())
            return;

        const float now = (float)Engine::GetTimeStep().GetElapsedSeconds();
        if(m_CosmoFade < 1.0f)
            m_CosmoFade = Maths::Clamp(m_CosmoFade + (now - m_CosmoLastTime) / Maths::Max(cosmo.Sky.FadeSeconds, 0.01f), 0.0f, 1.0f);
        m_CosmoLastTime = now;

        // A new region waits for the last fade to end
        if(cosmo.Version == m_CosmoBakedVersion || m_CosmoFade < 1.0f)
            return;

        LUMOS_PROFILE_GPU("Cosmo Bake");

        if(!m_CosmoSky[0])
        {
            Graphics::TextureDesc desc;
            desc.format          = Graphics::RHIFormat::R16G16B16A16_Float;
            desc.minFilter       = Graphics::TextureFilter::LINEAR;
            desc.magFilter       = Graphics::TextureFilter::LINEAR;
            desc.wrap            = Graphics::TextureWrap::REPEAT;
            desc.flags           = TextureFlags::Texture_RenderTarget;
            desc.generateMipMaps = false;
            m_CosmoSky[0]        = Graphics::Texture2D::Create(desc, 2048, 1024);
            m_CosmoSky[1]        = Graphics::Texture2D::Create(desc, 2048, 1024);
        }

        const bool first     = m_CosmoBakedVersion == 0;
        const int target     = first ? 0 : 1 - m_CosmoSkyFront;
        m_CosmoBakedVersion  = cosmo.Version;
        const auto& r        = cosmo.Sky;
        Mat4 identity[2]     = { Mat4(1.0f), Mat4(1.0f) };
        m_CosmoBakeDescriptorSet->SetUniformBufferData(0, identity);

        struct BakeData
        {
            Vec4 CamSeed, Params, Region, Pole, Centre, ColA, ColB, ColC, Tint;
        } data;
        data.CamSeed = Vec4(r.Offset.x, r.Offset.y, r.Offset.z, r.Seed);
        data.Params  = Vec4(r.Nebula, r.Band, r.Scale, r.Wander);
        data.Region  = Vec4(r.Tightness, r.Bulge, r.Lanes, r.Density);
        data.Pole    = Vec4(r.Pole.x, r.Pole.y, r.Pole.z, 0.0f);
        data.Centre  = Vec4(r.Centre.x, r.Centre.y, r.Centre.z, 0.0f);
        data.ColA    = Vec4(r.ColourA.x, r.ColourA.y, r.ColourA.z, 0.0f);
        data.ColB    = Vec4(r.ColourB.x, r.ColourB.y, r.ColourB.z, 0.0f);
        data.ColC    = Vec4(r.ColourC.x, r.ColourC.y, r.ColourC.z, 0.0f);
        data.Tint    = Vec4(r.Tint.x, r.Tint.y, r.Tint.z, 0.0f);
        m_CosmoBakeDescriptorSet->SetUniformBufferData(1, &data);
        m_CosmoBakeDescriptorSet->Update();

        Graphics::PipelineDesc pd {};
        pd.shader              = m_CosmoBakeShader;
        pd.polygonMode         = Graphics::PolygonMode::FILL;
        pd.cullMode            = Graphics::CullMode::NONE;
        pd.transparencyEnabled = false;
        pd.clearTargets        = true;
        pd.colourTargets[0]    = m_CosmoSky[target];
        pd.DebugName           = "CosmoBake";

        auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
        auto pipeline      = Graphics::Pipeline::Get(pd);
        commandBuffer->BindPipeline(pipeline);
        auto set = m_CosmoBakeDescriptorSet.get();
        Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
        Renderer::DrawMesh(commandBuffer, pipeline.get(), m_ScreenQuad);
        commandBuffer->UnBindPipeline();

        if(!first)
            m_CosmoSkyBothBaked = true;
        m_CosmoSkyFront = target;
        m_CosmoFade     = first ? 1.0f : 0.0f;

        Texture* front = m_CosmoSky[target];
        Texture* back  = m_CosmoSkyBothBaked ? m_CosmoSky[1 - target] : front;
        m_CosmoDescriptorSet->SetTexture(2, front);
        m_CosmoDescriptorSet->SetTexture(3, back);
        m_CosmoDescriptorSet->TransitionImages(commandBuffer);
    }

    void SceneRenderer::CosmoPass()
    {
        LUMOS_PROFILE_FUNCTION();

        auto& cosmo = Graphics::CosmoBackdrop::Get();
        if(!cosmo.Enabled || m_CosmoBakedVersion == 0)
            return;
        if(!m_CosmoShader || !m_CosmoShader->IsCompiled())
            return;
        if(!m_Camera || !m_CameraTransform)
            return;

        LUMOS_PROFILE_GPU("Cosmo Pass");

        Mat4 view = Mat4::Inverse(m_CameraTransform->GetWorldMatrix());
        Mat4 canonicalProj = Mat4::Perspective(0.1f, 1000.0f, m_Camera->GetAspectRatio(), m_Camera->GetFOV());
        Mat4 matrices[2]   = { canonicalProj.Inverse(), view.Inverse() };
        m_CosmoDescriptorSet->SetUniformBufferData(0, matrices);

        Vec4 params(m_CosmoFade, cosmo.Gain, 0.0f, 0.0f);
        m_CosmoDescriptorSet->SetUniformBufferData(1, &params);
        m_CosmoDescriptorSet->Update();

        Graphics::PipelineDesc pipelineDesc {};
        pipelineDesc.shader              = m_CosmoShader;
        pipelineDesc.polygonMode         = Graphics::PolygonMode::FILL;
        pipelineDesc.cullMode            = Graphics::CullMode::BACK;
        pipelineDesc.transparencyEnabled = false;
        pipelineDesc.depthTarget         = reinterpret_cast<Texture*>(m_ForwardData.m_DepthTexture);
        pipelineDesc.colourTargets[0]    = m_MainTexture;
        pipelineDesc.DebugName           = "Cosmo";
        pipelineDesc.samples             = m_MainTextureSamples;
        if(m_MainTextureSamples > 1)
            pipelineDesc.resolveTexture = m_ResolveTexture;

        auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
        auto pipeline      = Graphics::Pipeline::Get(pipelineDesc);
        commandBuffer->BindPipeline(pipeline);

        auto set = m_CosmoDescriptorSet.get();
        Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
        Renderer::DrawMesh(commandBuffer, pipeline.get(), m_ScreenQuad);
    }

    void SceneRenderer::SkyboxPass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("SkyBox Pass");

        if(!m_CubeMap)
            return;

        if(!m_Camera || !m_CameraTransform)
            return;

        struct SkyboxUniformData
        {
            float Exposure;
            int Mode; // 0 env map , 1 custom sky
            float BlurLevel;
            float Time;
            Vec4 HorizonColour;
            Vec4 ZenithColour;
            Vec4 SunDirection; // xyz towards the sun
            Vec4 SunColour;    // rgb illuminance / kSkyUnit
            Vec4 FogColour;
            Vec4 FogParams;
            Vec4 CloudColour;
            Vec4 CloudParams;
            Vec4 CloudWindDir;
            Vec4 StarParams;   // x density (0 disables), y brightness, z twinkle speed, w sky-luma threshold
            Vec4 StarColour;   // rgb tint, a unused
            Vec4 AuroraColour; // rgb base tint, a tip-hue blend
            Vec4 AuroraParams; // x intensity (0 disables), y centre, z speed, w width
        };
        SkyboxUniformData data;
        data.Mode      = 0;
        data.Exposure  = m_Exposure * kSkyUnit;
        data.BlurLevel = m_CurrentScene->GetSettings().RenderSettings.SkyboxMipLevel;
        // Engine elapsed seconds — drives cloud scroll. Cheap accumulator on Engine.
        data.Time      = (float)Engine::GetTimeStep().GetElapsedSeconds();
        // Defaults if no Environment component is present.
        // sun and tints as the lighting bake sees them
        const SkyBakeData sky = BuildSkyBake(m_CurrentScene, Vec4(0.0f));
        data.HorizonColour    = sky.HorizonTint;
        data.ZenithColour     = sky.ZenithTint;
        data.SunDirection     = sky.SunDirection;
        data.SunColour        = sky.SunColour;
        data.FogColour     = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
        // Default linear horizon band 0.0..0.25 so the sky picks up fog tint when set.
        data.FogParams     = Vec4(0.0f, 0.0f, 0.0f, 0.25f);
        data.CloudColour   = Vec4(1.0f, 1.0f, 1.0f, 0.45f);
        data.CloudParams   = Vec4(0.5f, 0.7f, 0.02f, -1.0f); // styleMode -1 = off
        data.CloudWindDir  = Vec4(1.0f, 0.0f, 0.2f, 0.5f);
        data.StarParams    = Vec4(0.0f, 1.0f, 0.0f, 0.35f);
        data.StarColour    = Vec4(0.95f, 0.95f, 1.0f, 1.0f);
        data.AuroraColour  = Vec4(0.2f, 0.9f, 0.5f, 0.5f);
        data.AuroraParams  = Vec4(0.0f, 0.45f, 0.3f, 0.35f); // intensity 0 = off

        if(m_CurrentScene)
        {
            auto& reg     = m_CurrentScene->GetRegistry();
            auto envView  = reg.view<Graphics::Environment>();
            if(!envView.empty())
            {
                auto& env          = envView.get<Graphics::Environment>(envView.front());
                data.Mode          = env.GetMode();
                data.FogColour     = env.GetFogColour();
                data.FogParams     = env.GetFogParams();
                data.CloudColour   = env.GetCloudColour();
                data.CloudParams   = env.GetCloudParams();
                data.CloudWindDir  = env.GetCloudWindDir();
                data.StarParams    = env.GetStarParams();
                data.StarColour    = env.GetStarColour();
                data.AuroraColour  = env.GetAuroraColour();
                data.AuroraParams  = env.GetAuroraParams();
            }
            // the volumetric pass fogs the sky too
            if(m_CurrentScene->GetSettings().RenderSettings.VolumetricFogEnabled && !m_DisablePostProcess)
                data.FogColour.w = 0.0f;
        }
        // a blurred sky reads the lighting cube's mips; a sharp one the full-detail cube
        Texture* skyCube = data.BlurLevel <= 0.0f ? SkyboxCubeFor(m_CubeMap) : m_CubeMap;
        m_SkyboxDescriptorSet->SetTexture(1, skyCube, 0, TextureType::CUBE);
        m_SkyboxDescriptorSet->SetTexture(3, GetSkyNoiseTexture());
        m_SkyboxDescriptorSet->SetUniformBufferData(2, &data);
        m_SkyboxDescriptorSet->Update();

        Graphics::PipelineDesc pipelineDesc {};
        pipelineDesc.shader = m_SkyboxShader;

        pipelineDesc.polygonMode         = Graphics::PolygonMode::FILL;
        pipelineDesc.cullMode            = Graphics::CullMode::NONE;
        pipelineDesc.transparencyEnabled = false;

        {
            pipelineDesc.depthTarget = reinterpret_cast<Texture*>(m_ForwardData.m_DepthTexture);
        }

        pipelineDesc.colourTargets[0] = m_MainTexture; // m_MainTexture;
        pipelineDesc.DebugName        = "Skybox";
        pipelineDesc.samples          = m_MainTextureSamples;
        if(m_MainTextureSamples > 1)
            pipelineDesc.resolveTexture = m_ResolveTexture;
        auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
        auto pipeline      = Graphics::Pipeline::Get(pipelineDesc);
        commandBuffer->BindPipeline(pipeline);

        auto set = m_SkyboxDescriptorSet.get();
        Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
        // a grid made in the vertex shader (no buffers): Skybox.vert's GRID_X x GRID_Y
        Renderer::Draw(commandBuffer, DrawType::TRIANGLE, kSkyGridX * kSkyGridY * 6);
    }

    void SceneRenderer::DepthOfFieldPass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("Depth Of Field Pass");

        if(m_MainTextureSamples > 1)
        {
            // LWARN("Depth of field pass currently not working with msaa");
            return;
        }

        if(!m_Camera || !m_DepthOfFieldShader || !m_DepthOfFieldShader->IsCompiled())
            return;
        auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
        commandBuffer->UnBindPipeline();

        const Mat4 proj                            = m_Camera->GetGPUProjectionMatrix();
        Scene::SceneRenderSettings& renderSettings = m_CurrentScene->GetSettings().RenderSettings;

        float depthLinearizeMul = (-proj.Get(2, 3)); //[3][2]); // float depthLinearizeMul = ( clipFar * clipNear ) / ( clipFar - clipNear );
        float depthLinearizeAdd = (proj.Get(2, 2));  //[2][2]);  // float depthLinearizeAdd = clipFar / ( clipFar - clipNear );
        // correct the handedness issue.
        if(depthLinearizeMul * depthLinearizeAdd < 0)
            depthLinearizeAdd = -depthLinearizeAdd;

        Vec4 UniformData = { renderSettings.DepthOfFieldDistance, renderSettings.DepthOfFieldStrength, depthLinearizeMul, depthLinearizeAdd };

        m_DepthOfFieldPassDescriptorSet->SetUniformBufferData(0, &UniformData);
        m_DepthOfFieldPassDescriptorSet->SetTexture(1, m_LastRenderTarget);
        m_DepthOfFieldPassDescriptorSet->SetTexture(2, m_ForwardData.m_DepthTexture);
        m_DepthOfFieldPassDescriptorSet->Update();

        Graphics::PipelineDesc pipelineDesc {};
        pipelineDesc.shader              = m_DepthOfFieldShader;
        pipelineDesc.polygonMode         = Graphics::PolygonMode::FILL;
        pipelineDesc.cullMode            = Graphics::CullMode::BACK;
        pipelineDesc.transparencyEnabled = false;
        pipelineDesc.colourTargets[0]    = m_PostProcessTexture1;
        pipelineDesc.DebugName           = "DepthofField";
        pipelineDesc.samples             = m_MainTextureSamples;
        auto pipeline                    = Graphics::Pipeline::Get(pipelineDesc);
        commandBuffer->BindPipeline(pipeline);

        auto set = m_DepthOfFieldPassDescriptorSet.get();
        Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
        Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);

        Swap(m_LastRenderTarget, m_PostProcessTexture1);
    }

    void SceneRenderer::SharpenPass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("Sharpen Pass");

        if(!m_Camera || !m_SharpenShader || !m_SharpenShader->IsCompiled())
            return;

        Scene::SceneRenderSettings& renderSettings = m_CurrentScene->GetSettings().RenderSettings;

        m_SharpenPassDescriptorSet->SetTexture(1, m_LastRenderTarget);
        m_SharpenPassDescriptorSet->Update();

        Graphics::PipelineDesc pipelineDesc {};
        pipelineDesc.shader              = m_SharpenShader;
        pipelineDesc.polygonMode         = Graphics::PolygonMode::FILL;
        pipelineDesc.cullMode            = Graphics::CullMode::BACK;
        pipelineDesc.transparencyEnabled = false;
        pipelineDesc.colourTargets[0]    = m_PostProcessTexture1;
        pipelineDesc.DebugName           = "Sharpen";
        auto commandBuffer               = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
        auto pipeline                    = Graphics::Pipeline::Get(pipelineDesc);
        commandBuffer->BindPipeline(pipeline);

        auto set = m_SharpenPassDescriptorSet.get();
        Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
        Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);

        Swap(m_PostProcessTexture1, m_LastRenderTarget);
    }


    void SceneRenderer::draw_ui(UI_Widget* widget)
    {
        const Vec4& clip = widget->ClipRect;
        if(clip.z <= clip.x || clip.w <= clip.y)
            return;

        if(m_UIDeferredText)
        {
            // Quads now, text once the layer is done: one switch between the two
            // passes per layer instead of one per widget. A floating widget with a
            // fill can sit over text built before it, so that text goes down first.
            const u32 floating = WidgetFlags_Floating_X | WidgetFlags_Floating_Y;
            if((widget->flags & floating) == floating
               && (widget->flags & (WidgetFlags_DrawBackground | WidgetFlags_DrawBorder))
               && !m_UIDeferredText->Empty())
                FlushDeferredUIText();

            draw_ui_widget(widget, UIDrawPart_Quads);
            if((widget->flags & WidgetFlags_DrawText) && widget->text.size > 0)
                m_UIDeferredText->PushBack(widget);
        }
        else
        {
            draw_ui_widget(widget, UIDrawPart_All);
        }

        for(UI_Widget* child = widget->first; child; child = child->next)
        {
            draw_ui(child);
        }

        // each top-level panel is its own layer
        if(m_UIDeferredText && widget->parent && widget->parent->parent == nullptr)
            FlushDeferredUIText();
    }

    void SceneRenderer::FlushDeferredUIText()
    {
        if(!m_UIDeferredText)
            return;
        for(UI_Widget* widget : *m_UIDeferredText)
            draw_ui_widget(widget, UIDrawPart_Text);
        m_UIDeferredText->Clear();
    }

    void SceneRenderer::draw_ui_widget(UI_Widget* widget, int part)
    {
        // Inherited clip rect (layout space, y-down; ±1e9 = unclipped).
        const Vec4 clipRect = widget->ClipRect;
        if(clipRect.z <= clipRect.x || clipRect.w <= clipRect.y)
            return;

        // Fully outside the clip rect: nothing would land on screen, and the
        // quads/glyphs are clipped away later anyway - but switching between the
        // quad and text passes costs a batch flush per widget either way. A long
        // scrolled list would burn through every batch draw call in the frame.
        // Rotated widgets are culled by the layout, not clipped, so leave them.
        if(widget->Rotation == 0.0f)
        {
            const float margin = 1.0f;
            if(widget->position.x - margin >= clipRect.z
               || widget->position.y - margin >= clipRect.w
               || widget->position.x + widget->size.x + margin <= clipRect.x
               || widget->position.y + widget->size.y + margin <= clipRect.y)
                return;
        }

        // Apply easing to transitions for smoother visual feedback
        float easedHotTransition    = widget->HotTransition > 0.0f ? EaseOutCubic(widget->HotTransition) : 0.0f;
        float easedActiveTransition = widget->ActiveTransition > 0.0f ? EaseOutCubic(widget->ActiveTransition) : 0.0f;

        Vec4 border_color     = widget->style_vars[StyleVar_BorderColor];
        Vec4 background_color = widget->style_vars[StyleVar_BackgroundColor];
        Vec4 text_color       = widget->style_vars[StyleVar_TextColor];

        if(easedHotTransition > 0.0f)
        {
            Vec4 hotBorderColor     = widget->style_vars[StyleVar_HotBorderColor];
            Vec4 hotBackgroundColor = widget->style_vars[StyleVar_HotBackgroundColor];
            Vec4 hotTextColor       = widget->style_vars[StyleVar_HotTextColor];

            border_color     = border_color.Lerp(hotBorderColor, easedHotTransition);
            background_color = background_color.Lerp(hotBackgroundColor, easedHotTransition);
            text_color       = text_color.Lerp(hotTextColor, easedHotTransition);
        }

        if(easedActiveTransition > 0.0f)
        {
            Vec4 activeBorderColor     = widget->style_vars[StyleVar_ActiveBorderColor];
            Vec4 activeBackgroundColor = widget->style_vars[StyleVar_ActiveBackgroundColor];
            Vec4 activeTextColor       = widget->style_vars[StyleVar_ActiveTextColor];

            border_color     = border_color.Lerp(activeBorderColor, easedActiveTransition);
            background_color = background_color.Lerp(activeBackgroundColor, easedActiveTransition);
            text_color       = text_color.Lerp(activeTextColor, easedActiveTransition);
        }

        // Combined render scale: press effect + appear pop-in
        float renderScale = 1.0f;
        if((widget->flags & WidgetFlags_AnimateScale) && widget->ScaleAnimation < 1.0f)
            renderScale *= widget->ScaleAnimation;

        // Clamped: a caller pushing >1 would over-brighten and a negative would
        // flip the colour, neither of which is a useful opacity.
        float widgetAlpha = Maths::Clamp(widget->style_vars[StyleVar_Alpha].x, 0.0f, 1.0f);
        if((widget->flags & WidgetFlags_AnimateAppear) && widget->AppearTransition < 1.0f)
        {
            // Ease-out-back: overshoots ~10% then settles — classic pop-in.
            float t  = widget->AppearTransition;
            float c1 = 1.70158f;
            float c3 = c1 + 1.0f;
            float e  = 1.0f + c3 * powf(t - 1.0f, 3.0f) + c1 * powf(t - 1.0f, 2.0f);
            renderScale *= Maths::Max(e, 0.0f);
            widgetAlpha *= Maths::Clamp(t * 2.0f, 0.0f, 1.0f); // fade-in leads the scale
        }

        if(widget->ExitTransition > 0.0f)
        {
            float t = 1.0f - EaseOutCubic(widget->ExitTransition);
            widgetAlpha *= t;
            renderScale *= 0.92f + 0.08f * t; // slight shrink as it goes
        }

        // Invisible: emit nothing at all. Every drawn widget costs a switch
        // between the quad and text passes (a flush and a descriptor set each
        // way), so a faded-out subtree would otherwise burn the frame's batch
        // draw calls on pixels nobody sees. Children draw themselves - they
        // carry the same pushed alpha and drop out here too.
        if(widgetAlpha <= 0.003f)
            return;

        if(widgetAlpha < 1.0f)
        {
            border_color.w     *= widgetAlpha;
            background_color.w *= widgetAlpha;
            // text blends premultiplied, so the fade has to reach its rgb
            text_color.x       *= widgetAlpha;
            text_color.y       *= widgetAlpha;
            text_color.z       *= widgetAlpha;
            text_color.w       *= widgetAlpha;
        }

        // LUMOS_UI_ALPHA_DUMP=<frames>: log the resolved alpha/colour of every
        // background quad for the first N frames. For chasing panels that paint
        // faint - compare a good state against a bad one.
        {
            static int s_AlphaDumpFrames = []
            {
                const char* v = std::getenv("LUMOS_UI_ALPHA_DUMP");
                return v ? atoi(v) : 0;
            }();

            if(s_AlphaDumpFrames > 0 && (widget->flags & (WidgetFlags_DrawBackground | WidgetFlags_DrawBorder)))
            {
                static u64 s_LastFrame = ~0ull;
                static int s_Dumped    = 0;
                u64 frame              = GetUIState()->FrameIndex;
                if(frame != s_LastFrame)
                {
                    s_LastFrame = frame;
                    s_Dumped++;
                }

                if(s_Dumped <= s_AlphaDumpFrames)
                {
                    LINFO("[uialpha] f%llu '%.*s' pos=%.0f,%.0f size=%.0fx%.0f alpha=%.3f bg=%.2f,%.2f,%.2f,%.3f radius=%.1f clip=%.0f,%.0f,%.0f,%.0f flags=%#x",
                          (unsigned long long)frame,
                          (int)widget->text.size, (const char*)widget->text.str,
                          widget->position.x, widget->position.y, widget->size.x, widget->size.y,
                          widgetAlpha,
                          background_color.x, background_color.y, background_color.z, background_color.w,
                          widget->style_vars[StyleVar_CornerRadius].x,
                          clipRect.x, clipRect.y, clipRect.z, clipRect.w,
                          widget->flags);
                }
            }
        }

        Vec2 scaledSize = widget->size;
        Vec2 scaledPos = widget->position;
        if(renderScale != 1.0f)
        {
            Vec2 center = widget->position + widget->size * 0.5f;
            scaledSize = widget->size * renderScale;
            scaledPos = center - scaledSize * 0.5f;
        }

        const float uiHeight = m_UIProjectionSize.y > 0.0f ? m_UIProjectionSize.y : (float)m_MainTexture->GetHeight();

        auto emitClippedQuad = [&](const Vec2& posLayout, const Vec2& quadSize, const Vec4& colour, float textureSlot, float cornerRadius)
        {
            if(quadSize.x <= 0.0f || quadSize.y <= 0.0f)
                return;

            const float cl = Maths::Max(posLayout.x, clipRect.x);
            const float ct = Maths::Max(posLayout.y, clipRect.y);
            const float cr = Maths::Min(posLayout.x + quadSize.x, clipRect.z);
            const float cb = Maths::Min(posLayout.y + quadSize.y, clipRect.w);
            if(cr <= cl || cb <= ct)
                return;

            const float fx0 = (cl - posLayout.x) / quadSize.x;
            const float fx1 = (cr - posLayout.x) / quadSize.x;
            const float fy0 = (ct - posLayout.y) / quadSize.y;
            const float fy1 = (cb - posLayout.y) / quadSize.y;

            m_Stats.NumRenderedObjects++;

            if(m_Renderer2DData.m_IndexCount >= m_Renderer2DData.m_Limits.IndiciesSize)
            {
                Render2DFlush();
                Renderer2DBeginBatch();
            }

            // v=0 at the top: textures are stored top row first (same as the rotated path)
            const bool flipV = widget->FlipV;
            auto uvAt = [&](float fx, float fy) { return Vec2(fx, flipV ? 1.0f - fy : fy); };

            const float topY = uiHeight - ct;
            const float botY = uiHeight - cb;

            Vec2 c0 = uvAt(fx0, fy0);
            m_Renderer2DData.m_Buffer->vertex = Vec3(cl, topY, 0.0f);
            m_Renderer2DData.m_Buffer->uv     = Vec4(c0.x, c0.y, quadSize.x, quadSize.y);
            m_Renderer2DData.m_Buffer->tid    = Vec2(textureSlot, cornerRadius);
            m_Renderer2DData.m_Buffer->colour = colour;
            m_Renderer2DData.m_Buffer++;

            Vec2 c1 = uvAt(fx0, fy1);
            m_Renderer2DData.m_Buffer->vertex = Vec3(cl, botY, 0.0f);
            m_Renderer2DData.m_Buffer->uv     = Vec4(c1.x, c1.y, quadSize.x, quadSize.y);
            m_Renderer2DData.m_Buffer->tid    = Vec2(textureSlot, cornerRadius);
            m_Renderer2DData.m_Buffer->colour = colour;
            m_Renderer2DData.m_Buffer++;

            Vec2 c2 = uvAt(fx1, fy1);
            m_Renderer2DData.m_Buffer->vertex = Vec3(cr, botY, 0.0f);
            m_Renderer2DData.m_Buffer->uv     = Vec4(c2.x, c2.y, quadSize.x, quadSize.y);
            m_Renderer2DData.m_Buffer->tid    = Vec2(textureSlot, cornerRadius);
            m_Renderer2DData.m_Buffer->colour = colour;
            m_Renderer2DData.m_Buffer++;

            Vec2 c3 = uvAt(fx1, fy0);
            m_Renderer2DData.m_Buffer->vertex = Vec3(cr, topY, 0.0f);
            m_Renderer2DData.m_Buffer->uv     = Vec4(c3.x, c3.y, quadSize.x, quadSize.y);
            m_Renderer2DData.m_Buffer->tid    = Vec2(textureSlot, cornerRadius);
            m_Renderer2DData.m_Buffer->colour = colour;
            m_Renderer2DData.m_Buffer++;

            m_Renderer2DData.m_IndexCount += 6;
        };

        // Invisible fills still cost a full blended quad each (containers, clear borders): skip them.
        constexpr float kMinVisibleAlpha = 1.0f / 512.0f;
        const bool drawQuads      = part != UIDrawPart_Text;
        const bool drawBorder     = drawQuads && (widget->flags & WidgetFlags_DrawBorder) && border_color.w > kMinVisibleAlpha;
        const bool drawBackground = drawQuads && (widget->flags & WidgetFlags_DrawBackground)
                                    && (background_color.w > kMinVisibleAlpha || (widget->WarpStrips > 0 && widget->WarpColourBottom.w * widgetAlpha > kMinVisibleAlpha));

        if(drawBorder)
        {
            Vec2 size = scaledSize;
            Vec2 p    = scaledPos;

            if(m_CurrentUIText)
            {
                if(m_TextRendererData.m_IndexCount > 0)
                {
                    TextFlush(m_TextRendererData, TextVertexBufferBase, TextVertexBufferPtr);
                    // BeginTextPass();
                }

                Begin2DPass();
            }

            m_CurrentUIText = false;

            {
                Texture* texture  = widget->texture;
                float textureSlot = 0.0f;
                if(texture)
                    textureSlot = SubmitTexture(texture) + kUITextureRaw; // UI images are already display colours

                float cornerRadius = widget->style_vars[StyleVar_CornerRadius].x;

                emitClippedQuad(p, size, border_color, textureSlot, cornerRadius);
            }
        }

        if(drawBackground)
        {
            Vec2 border = widget->style_vars[StyleVar_Border].ToVector2();
            // Inset background by border width for proper layering
            Vec2 size   = scaledSize - border * 2.0f;
            Vec2 p      = scaledPos + border;

            if(widget->RenderSize.x > 0.0f && widget->RenderSize.y > 0.0f)
            {
                p    = scaledPos + (scaledSize - widget->RenderSize) * 0.5f;
                size = widget->RenderSize;
            }

            if(m_CurrentUIText)
            {
                if(m_TextRendererData.m_IndexCount > 0)
                {
                    TextFlush(m_TextRendererData, TextVertexBufferBase, TextVertexBufferPtr);
                    // BeginTextPass();
                }

                Begin2DPass();
            }

            m_CurrentUIText = false;
            {
                const Vec4 colour = background_color;
                const auto& uv    = Renderable2D::GetDefaultUVs();
                Texture* texture  = widget->texture;

                float textureSlot = 0.0f;
                if(texture)
                    textureSlot = SubmitTexture(texture) + kUITextureRaw; // UI images are already display colours

                float cornerRadius = widget->style_vars[StyleVar_CornerRadius].x;

                if(widget->WarpStrips > 0)
                {
                    // Arbitrary quad split into vertical strips; u is spaced perspective-correctly
                    // between the edge depths, so each strip only needs affine interpolation.
                    const Vec2 base  = widget->position;
                    const Vec2* c    = widget->WarpCorners; // tl, tr, br, bl
                    const float wl   = 1.0f / widget->WarpDepth[0];
                    const float wr   = 1.0f / widget->WarpDepth[1];
                    Vec4 bottomColour = widget->WarpColourBottom;
                    bottomColour.w *= widgetAlpha;

                    auto edge = [&](const Vec2& a, const Vec2& b, float u) {
                        const float x = (1.0f - u) * wl, y = u * wr;
                        return base + (a * x + b * y) / (x + y);
                    };
                    auto uvAt = [&](float u, float v) {
                        return Vec2(u, widget->WarpFlipV ? 1.0f - v : v);
                    };

                    Vec2 mn(FLT_MAX), mx(-FLT_MAX);
                    for(int i = 0; i < 4; i++)
                    {
                        mn = Vec2(Maths::Min(mn.x, base.x + c[i].x), Maths::Min(mn.y, base.y + c[i].y));
                        mx = Vec2(Maths::Max(mx.x, base.x + c[i].x), Maths::Max(mx.y, base.y + c[i].y));
                    }
                    if(!(mx.x < clipRect.x || mn.x > clipRect.z || mx.y < clipRect.y || mn.y > clipRect.w))
                    {
                        m_Stats.NumRenderedObjects++;
                        // uv spans the whole quad, so one rounded-rect SDF covers every strip
                        const Vec2 warpSize = mx - mn;
                        auto push = [&](const Vec2& v, const Vec2& t, const Vec4& col) {
                            m_Renderer2DData.m_Buffer->vertex = Vec3(v.x, uiHeight - v.y, 0.0f);
                            m_Renderer2DData.m_Buffer->uv     = Vec4(t.x, t.y, warpSize.x, warpSize.y);
                            m_Renderer2DData.m_Buffer->tid    = Vec2(textureSlot, cornerRadius);
                            m_Renderer2DData.m_Buffer->colour = col;
                            m_Renderer2DData.m_Buffer++;
                        };
                        const u32 n = widget->WarpStrips;
                        if(widget->WarpVertical)
                        {
                            // depths are top / bottom: horizontal strips, v spaced perspective-correctly
                            for(u32 k = 0; k < n; k++)
                            {
                                if(m_Renderer2DData.m_IndexCount >= m_Renderer2DData.m_Limits.IndiciesSize)
                                {
                                    Render2DFlush();
                                    Renderer2DBeginBatch();
                                }
                                // v is the texture row; edge() puts it where perspective does (wl / wr = top / bottom here)
                                const float v0 = (float)k / n, v1 = (float)(k + 1) / n;
                                const Vec4 col0 = colour + (bottomColour - colour) * v0;
                                const Vec4 col1 = colour + (bottomColour - colour) * v1;
                                push(edge(c[0], c[3], v0), uvAt(0.0f, v0), col0);
                                push(edge(c[0], c[3], v1), uvAt(0.0f, v1), col1);
                                push(edge(c[1], c[2], v1), uvAt(1.0f, v1), col1);
                                push(edge(c[1], c[2], v0), uvAt(1.0f, v0), col0);
                                m_Renderer2DData.m_IndexCount += 6;
                            }
                        }
                        else
                        for(u32 k = 0; k < n; k++)
                        {
                            if(m_Renderer2DData.m_IndexCount >= m_Renderer2DData.m_Limits.IndiciesSize)
                            {
                                Render2DFlush();
                                Renderer2DBeginBatch();
                            }
                            const float u0 = (float)k / n, u1 = (float)(k + 1) / n;
                            push(edge(c[0], c[1], u0), uvAt(u0, 0.0f), colour);
                            push(edge(c[3], c[2], u0), uvAt(u0, 1.0f), bottomColour);
                            push(edge(c[3], c[2], u1), uvAt(u1, 1.0f), bottomColour);
                            push(edge(c[0], c[1], u1), uvAt(u1, 0.0f), colour);
                            m_Renderer2DData.m_IndexCount += 6;
                        }
                    }
                }
                else if(widget->Rotation != 0.0f)
                {
                    // Cull by axis-aligned bounds vs clip
                    if(p.x + size.x < clipRect.x || p.x > clipRect.z || p.y + size.y < clipRect.y || p.y > clipRect.w)
                    {
                        // fully outside — draw nothing (children still walk below)
                    }
                    else
                    {
                    m_Stats.NumRenderedObjects++;

                    if(m_Renderer2DData.m_IndexCount >= m_Renderer2DData.m_Limits.IndiciesSize)
                    {
                        Render2DFlush();
                        Renderer2DBeginBatch();
                    }

                    p.y = uiHeight - p.y;

                    const float cs = cosf(-widget->Rotation);
                    const float sn = sinf(-widget->Rotation);
                    const Vec2 c   = p + Vec2(size.x * 0.5f, -size.y * 0.5f);
                    const Vec2 hx  = Vec2(size.x * 0.5f * cs, size.x * 0.5f * sn);
                    const Vec2 hy  = Vec2(-size.y * 0.5f * sn, size.y * 0.5f * cs);
                    const Vec2 v00 = c - hx - hy;
                    const Vec2 v01 = c + hx - hy;
                    const Vec2 v11 = c + hx + hy;
                    const Vec2 v10 = c - hx + hy;

                    auto push = [&](const Vec2& v, const Vec2& u) {
                        m_Renderer2DData.m_Buffer->vertex = Vec4(v.x, v.y, 0.0f, 1.0f);
                        m_Renderer2DData.m_Buffer->uv     = Vec4(u.x, u.y, size.x, size.y);
                        m_Renderer2DData.m_Buffer->tid    = Vec2(textureSlot, cornerRadius);
                        m_Renderer2DData.m_Buffer->colour = colour;
                        m_Renderer2DData.m_Buffer++;
                    };
                    // CCW vertex order to match the non-rotated path below.
                    push(v00, uv[0]);
                    push(v10, uv[3]);
                    push(v11, uv[2]);
                    push(v01, uv[1]);
                    m_Renderer2DData.m_IndexCount += 6;
                    }
                }
                else
                {
                    (void)uv;
                    emitClippedQuad(p, size, colour, textureSlot, cornerRadius);
                }
            }
        }

        if((widget->flags & WidgetFlags_DrawText) && part != UIDrawPart_Quads)
        {
            Vec2 padding   = widget->style_vars[StyleVar_Padding].ToVector2();
            // Glyphs follow the pop-in/press scale so text pops with its panel
            float fontSize = widget->style_vars[StyleVar_FontSize].x * renderScale;

            Vec2 size = scaledSize;
            Vec2 p    = scaledPos;

            Vec2 textSize = GetStringSizeFont(widget->text, fontSize, widget->MonoText);
            p.y = uiHeight - p.y;

            float extraLinesH = 0.0f;
            {
                int nLines = 0;
                for(int ci = 0; ci < widget->text.size; ci++)
                    if(widget->text.str[ci] == '\n')
                        nLines++;
                if(nLines > 0)
                {
                    const auto& met = (widget->MonoText ? Font::GetMonoFont() : Font::GetDefaultFont())->GetMSDFData()->FontGeometry.getMetrics();
                    const float fs  = 1.0f / (float)(met.ascenderY - met.descenderY);
                    extraLinesH     = (float)nLines * fs * (float)met.lineHeight * fontSize;
                }
            }

            // X positioning
            if(widget->TextAlignment & UI_Text_Alignment_Center_X)
                p.x += (size.x - textSize.x) * 0.5f;
            else
                p.x += padding.x;

            // Y positioning (single-line unchanged: extraLinesH == 0)
            if(widget->TextAlignment & UI_Text_Alignment_Center_Y)
                p.y -= (size.y + textSize.y) * 0.5f - extraLinesH;
            else
                p.y -= size.y - padding.y - extraLinesH;
            if(!m_CurrentUIText)
            {
                if(m_Renderer2DData.m_IndexCount > 0)
                {
                    Render2DFlush();
                }
                BeginTextPass();
            }

            m_CurrentUIText = true;

            // whole-pixel baseline: a fractional one smears horizontal strokes over two rows
            Vec2 pos = Vec2(floorf(p.x + 0.5f), floorf(p.y + 0.5f));
            {
                const float clipL    = clipRect.x;
                const float clipR    = clipRect.z;
                const float clipBotF = uiHeight - clipRect.w;
                const float clipTopF = uiHeight - clipRect.y;

                m_Stats.NumRenderedObjects++;

                if(m_TextRendererData.m_IndexCount >= m_TextRendererData.m_Limits.IndiciesSize)
                {
                    TextFlush(m_TextRendererData, TextVertexBufferBase, TextVertexBufferPtr);
                    BeginTextPass();
                }

                int textureIndex       = -1;
                auto& string           = widget->text;
                auto font              = widget->MonoText ? Font::GetMonoFont() : Font::GetDefaultFont();
                float lineHeightOffset = 0.0f;
                float kerningOffset    = 0.0f;

                float maxWidth     = 100.0f;
                auto colour        = text_color;
                float lineSpacing  = 0.0f;
                float kerning      = 0.0f;
                auto outlineColour = Vec4(1.0f);
                auto outlineWidth  = 0.0f;

                SharedPtr<Texture2D> fontAtlas = font->GetFontAtlas();
                if(!fontAtlas)
                    ASSERT(false);

                for(uint32_t i = 0; i < m_TextRendererData.m_TextureCount; i++)
                {
                    if(m_TextRendererData.m_Textures[i] == fontAtlas.get())
                    {
                        textureIndex = int(i + 1);
                        break;
                    }
                }

                if(textureIndex == -1)
                {
                    textureIndex                                                     = (int)m_TextRendererData.m_TextureCount + 1;
                    m_TextRendererData.m_Textures[m_TextRendererData.m_TextureCount] = fontAtlas.get();
                    m_TextRendererData.m_TextureCount++;
                }

                auto& fontGeometry  = font->GetMSDFData()->FontGeometry;
                const auto& metrics = fontGeometry.getMetrics();

                {
                    double x       = 0.0;
                    double fsScale = 1 / (metrics.ascenderY - metrics.descenderY);
                    double y       = 0.0;
                    const MSDFData* msdfData = font->GetMSDFData();
                    const bool anyHighlight  = (widget->HighlightBits[0] | widget->HighlightBits[1]) != 0;
                    Vec4 highlightColour     = widget->HighlightColour;
                    if(widgetAlpha < 1.0f)
                        highlightColour.w *= widgetAlpha;
                    for(int i = 0; i < string.size; i++)
                    {
                        UnicodeDecode dec  = Utf8Decode(string.str + i, string.size - i);
                        char32_t character = dec.codepoint;
                        if(dec.inc > 1)
                            i += dec.inc - 1;

                        if(character == '\r')
                            continue;

                        if(character == '\n')
                        {
                            x = 0;
                            y -= fsScale * metrics.lineHeight + lineHeightOffset;
                            continue;
                        }

                        if(character == '\t')
                        {
                            auto glyph     = fontGeometry.getGlyph('a');
                            double advance = glyph->getAdvance();
                            x += 4 * fsScale * advance + kerningOffset;
                            continue;
                        }

                        auto glyph = msdfData->GetGlyph(character);
                        if(!glyph)
                            glyph = fontGeometry.getGlyph('?');
                        if(!glyph)
                            continue;

                        double l, b, r, t;
                        glyph->getQuadAtlasBounds(l, b, r, t);

                        double pl, pb, pr, pt;
                        glyph->getQuadPlaneBounds(pl, pb, pr, pt);

                        pl *= fsScale, pb *= fsScale, pr *= fsScale, pt *= fsScale;
                        pl += x, pb += y, pr += x, pt += y;

                        double texelWidth  = 1. / fontAtlas->GetWidth();
                        double texelHeight = 1. / fontAtlas->GetHeight();
                        l *= texelWidth, b *= texelHeight, r *= texelWidth, t *= texelHeight;

                        {
                            LUMOS_PROFILE_SCOPE("Set text buffer data");
                            Vec4 glyphColour = colour;
                            if(anyHighlight && i < 128 && (widget->HighlightBits[i >> 6] & (1ull << (i & 63))))
                                glyphColour = highlightColour;
                            const float gx0 = pos.x + (float)pl * fontSize;
                            const float gx1 = pos.x + (float)pr * fontSize;
                            const float gy0 = pos.y + (float)pb * fontSize;
                            const float gy1 = pos.y + (float)pt * fontSize;

                            const float cx0 = Maths::Max(gx0, clipL);
                            const float cx1 = Maths::Min(gx1, clipR);
                            const float cy0 = Maths::Max(gy0, clipBotF);
                            const float cy1 = Maths::Min(gy1, clipTopF);
                            if(cx1 <= cx0 || cy1 <= cy0)
                            {
                                // fully clipped — still advance the pen below
                            }
                            else
                            {
                                const float iw = 1.0f / (gx1 - gx0);
                                const float ih = 1.0f / (gy1 - gy0);
                                const float cl2 = (float)l + (float)(r - l) * (cx0 - gx0) * iw;
                                const float cr2 = (float)l + (float)(r - l) * (cx1 - gx0) * iw;
                                const float cb2 = (float)b + (float)(t - b) * (cy0 - gy0) * ih;
                                const float ct2 = (float)b + (float)(t - b) * (cy1 - gy0) * ih;

                                // one long label can hold more glyphs than the batch has room
                                // for (a compiler command line in a build log): flush mid-label
                                if(m_TextRendererData.m_IndexCount >= m_TextRendererData.m_Limits.IndiciesSize)
                                {
                                    TextFlush(m_TextRendererData, TextVertexBufferBase, TextVertexBufferPtr);
                                    BeginTextPass();
                                    textureIndex                                                     = (int)m_TextRendererData.m_TextureCount + 1;
                                    m_TextRendererData.m_Textures[m_TextRendererData.m_TextureCount] = fontAtlas.get();
                                    m_TextRendererData.m_TextureCount++;
                                }

                                TextVertexBufferPtr->vertex        = Vec3(cx0, cy0, 0.0f);
                                TextVertexBufferPtr->colour        = glyphColour;
                                TextVertexBufferPtr->uv            = { cl2, cb2 };
                                TextVertexBufferPtr->tid           = Vec2((float)textureIndex, outlineWidth);
                                TextVertexBufferPtr->outlineColour = outlineColour;
                                TextVertexBufferPtr++;

                                TextVertexBufferPtr->vertex        = Vec3(cx1, cy0, 0.0f);
                                TextVertexBufferPtr->colour        = glyphColour;
                                TextVertexBufferPtr->uv            = { cr2, cb2 };
                                TextVertexBufferPtr->tid           = Vec2((float)textureIndex, outlineWidth);
                                TextVertexBufferPtr->outlineColour = outlineColour;
                                TextVertexBufferPtr++;

                                TextVertexBufferPtr->vertex        = Vec3(cx1, cy1, 0.0f);
                                TextVertexBufferPtr->colour        = glyphColour;
                                TextVertexBufferPtr->uv            = { cr2, ct2 };
                                TextVertexBufferPtr->tid           = Vec2((float)textureIndex, outlineWidth);
                                TextVertexBufferPtr->outlineColour = outlineColour;
                                TextVertexBufferPtr++;

                                TextVertexBufferPtr->vertex        = Vec3(cx0, cy1, 0.0f);
                                TextVertexBufferPtr->colour        = glyphColour;
                                TextVertexBufferPtr->uv            = { cl2, ct2 };
                                TextVertexBufferPtr->tid           = Vec2((float)textureIndex, outlineWidth);
                                TextVertexBufferPtr->outlineColour = outlineColour;
                                TextVertexBufferPtr++;

                                m_TextRendererData.m_IndexCount += 6;
                            }
                        }

                        double advance = glyph->getAdvance();
                        char32_t next = (i + 1 < string.size) ? Utf8Decode(string.str + i + 1, string.size - i - 1).codepoint : 0;
                        fontGeometry.getAdvance(advance, character, next);
                        x += fsScale * advance + kerningOffset;
                    }
                }
            }
        }

    }

    void SceneRenderer::UIPass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("UI Pass");

        DrawUI(GetUIState());
    }

    void SceneRenderer::ScreenUIPass(UI_State* ui, bool standalone)
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("Screen UI Pass");

        if(!ui || !ui->root_parent.first)
            return;

        auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
        // whatever pass the scene left open ends here; this one draws on the window itself
        commandBuffer->UnBindPipeline();

        // No scene was drawn this frame, so nothing else has used the batches.
        if(standalone)
        {
            m_Renderer2DData.m_BatchDrawCallIndex   = 0;
            m_TextRendererData.m_BatchDrawCallIndex = 0;
        }

#ifdef LUMOS_RENDER_API_VULKAN
        // Render targets shown as images have to be readable before the pass starts -
        // a layout change is not allowed once it is open.
        for(UI_Widget* widget = &ui->root_parent; widget; widget = UIWidgetRecurseDepthFirstPreOrder(widget))
        {
            if(widget->texture)
                TransitionImageToCorrectLayout(widget->texture, commandBuffer);
        }
#endif

        m_UISwapchainTarget = true;
        DrawUI(ui);
        m_UISwapchainTarget = false;

        commandBuffer->UnBindPipeline();
    }

    void SceneRenderer::DrawUI(UI_State* ui)
    {
        m_UIProjectionSize = ui->FrameBufferSize;

        BeginTextPass();
        // Begin2DPass();

        m_CurrentUIText = true;

        TDArray<UI_Widget*> deferredText(ui->UIFrameArena);
        m_UIDeferredText = ui->LayeredBatching ? &deferredText : nullptr;

        draw_ui(&ui->root_parent);
        FlushDeferredUIText();
        m_UIDeferredText = nullptr;

        {
            TDArray<UI_Widget*> dying(ui->UIFrameArena);
            ForHashMapEach(u64, UI_Widget*, &ui->widgets, it)
            {
                UI_Widget* w = *it.value;
                if(w->LastFrameIndexActive < ui->FrameIndex && w->ExitTransition > 0.0f && w->ExitTransition < 1.0f)
                    dying.PushBack(w);
            }

            if(dying.Size() > 0)
            {
                for(u32 a = 1; a < dying.Size(); a++)
                {
                    UI_Widget* key = dying[a];
                    i32 b          = (i32)a - 1;
                    while(b >= 0 && dying[b]->ExitDepth > key->ExitDepth)
                    {
                        dying[b + 1] = dying[b];
                        b--;
                    }
                    dying[b + 1] = key;
                }

                for(u32 a = 0; a < dying.Size(); a++)
                    draw_ui_widget(dying[a], UIDrawPart_All);
            }
        }

        if(m_CurrentUIText && m_TextRendererData.m_IndexCount > 0)
            TextFlush(m_TextRendererData, TextVertexBufferBase, TextVertexBufferPtr);

        if(!m_CurrentUIText && m_Renderer2DData.m_IndexCount > 0)
            Render2DFlush();

        m_UIProjectionSize = Vec2(0.0f, 0.0f);
    }

    void SceneRenderer::ToneMappingPass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("Tone Mapping Pass");

        auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
        commandBuffer->UnBindPipeline();

        struct ToneMappingUniformData
        {
            float BloomIntensity;
            int ToneMapIndex;
            float Saturation;
            float Contrast;
            float Brightness;
            float TargetLuminance;
            float UseAdaptive;
            float KeepAlpha;
        };

        const auto& renderSettings = m_CurrentScene->GetSettings().RenderSettings;
        const bool useAdaptive     = renderSettings.AdaptiveExposure && m_AdaptiveExposureReady;

        ToneMappingUniformData data;
        data.BloomIntensity   = (m_BloomTextureLastRenderered && renderSettings.BloomEnabled && Application::Get().GetQualitySettings().EnableBloom) ? renderSettings.m_BloomIntensity : 0.0f;
        data.Saturation       = renderSettings.Saturation;
        data.Brightness       = renderSettings.Brightness;
        data.Contrast         = renderSettings.Contrast;
        data.ToneMapIndex     = m_ToneMapIndex;
        data.TargetLuminance  = renderSettings.AdaptiveTargetLuminance;
        data.UseAdaptive      = useAdaptive ? 1.0f : 0.0f;
        data.KeepAlpha        = m_TransparentBackground ? 1.0f : 0.0f;

        m_ToneMappingPassDescriptorSet->SetUniformBufferData(0, &data);
        m_ToneMappingPassDescriptorSet->SetTexture(1, m_LastRenderTarget);
        m_ToneMappingPassDescriptorSet->SetTexture(2, m_BloomTextureLastRenderered ? m_BloomTextureLastRenderered : Graphics::Material::GetDefaultTexture().get());
        m_ToneMappingPassDescriptorSet->SetStorageBuffer(3, m_AverageLuminanceSSBO);
        m_ToneMappingPassDescriptorSet->Update();

        Graphics::PipelineDesc pipelineDesc {};
        pipelineDesc.shader              = m_ToneMappingPassShader;
        pipelineDesc.polygonMode         = Graphics::PolygonMode::FILL;
        pipelineDesc.cullMode            = Graphics::CullMode::BACK;
        pipelineDesc.transparencyEnabled = false;
        pipelineDesc.colourTargets[0]    = m_PostProcessTexture1;
        pipelineDesc.DebugName           = "ToneMapping";
        auto pipeline                    = Graphics::Pipeline::Get(pipelineDesc);
        commandBuffer->BindPipeline(pipeline);

        auto set = m_ToneMappingPassDescriptorSet.get();
        Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
        Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);

        Swap(m_PostProcessTexture1, m_LastRenderTarget);
    }

    void SceneRenderer::FinalPass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("Final Pass");

        auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
        commandBuffer->UnBindPipeline();

        Texture* finalPassTexture = m_LastRenderTarget;

#ifndef LUMOS_DIST

        int debugMode = m_CurrentScene->GetSettings().RenderSettings.DebugMode;
        switch(debugMode)
        {
        case 1:
            finalPassTexture = m_SSAOTexture;
            break;
        case 2:
            finalPassTexture = m_SSAOTexture1;
            break;
        case 3:
            finalPassTexture = m_NormalTexture;
            break;
        case 4:
            finalPassTexture = m_BloomTexture;
            break;
        case 5:
            finalPassTexture = m_NoiseTexture;
            break;
        case 6:
            finalPassTexture = m_PostProcessTexture1;
            break;
        case 0:
        default:
            break;
        }
#endif

        const bool keepAlpha                    = m_TransparentBackground && m_FinalPassAlphaDescriptorSet;
        Graphics::DescriptorSet* finalPassSet   = keepAlpha ? m_FinalPassAlphaDescriptorSet.get() : m_FinalPassDescriptorSet.get();
        finalPassSet->SetTexture(1, finalPassTexture);
        finalPassSet->Update();

        Graphics::PipelineDesc pipelineDesc {};
        pipelineDesc.shader = keepAlpha ? m_FinalPassAlphaShader : m_FinalPassShader;

        pipelineDesc.polygonMode         = Graphics::PolygonMode::FILL;
        pipelineDesc.cullMode            = Graphics::CullMode::BACK;
        pipelineDesc.transparencyEnabled = false;
        pipelineDesc.DebugName           = "Final Pass";
        const bool swapchainTarget       = (m_ForwardData.m_RenderTexture == nullptr);
        if(m_ForwardData.m_RenderTexture)
            pipelineDesc.colourTargets[0] = m_ForwardData.m_RenderTexture;
        else
            pipelineDesc.swapchainTarget = true;

        SafeAreaInsets sa  = {};
        bool letterbox     = false;
        if(swapchainTarget
           && Application::Get().GetProjectSettings().SafeAreaMode == (int)Application::SafeAreaLayout::SafeArea
           && Application::Get().GetAppType() != AppType::Editor)
        {
            sa        = OS::Get().GetSafeAreaInsets();
            letterbox = (sa.left > 0.0f || sa.right > 0.0f || sa.top > 0.0f || sa.bottom > 0.0f);
        }
        if(letterbox)
        {
            pipelineDesc.clearTargets   = true;
            pipelineDesc.clearColour[0] = 0.0f;
            pipelineDesc.clearColour[1] = 0.0f;
            pipelineDesc.clearColour[2] = 0.0f;
            pipelineDesc.clearColour[3] = 1.0f;
        }

        auto pipeline = Graphics::Pipeline::Get(pipelineDesc);
        commandBuffer->BindPipeline(pipeline);

        if(letterbox)
        {
            Texture* sc = Renderer::GetMainSwapChain()->GetCurrentImage();
            const int32_t insetW = (int32_t)sc->GetWidth() - (int32_t)(sa.left + sa.right);
            const int32_t insetH = (int32_t)sc->GetHeight() - (int32_t)(sa.top + sa.bottom);
            // Swapchain viewport is flipped (matches BeginRenderPass for swapchain targets).
            commandBuffer->SetViewportRect((int32_t)sa.left, (int32_t)sa.top,
                                           (uint32_t)Maths::Max(insetW, 1), (uint32_t)Maths::Max(insetH, 1), true);
        }

        auto set = finalPassSet;
        Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
        Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);

        if(m_LastRenderTarget != m_MainTexture && m_LastRenderTarget != m_ResolveTexture)
            Swap(m_PostProcessTexture1, m_LastRenderTarget);

        commandBuffer->UnBindPipeline();
        commandBuffer->EndCurrentRenderPass();
    }

    void SceneRenderer::BloomPass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("Bloom Pass");

        auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
        commandBuffer->UnBindPipeline();

        if(!m_BloomPassShader->IsCompiled() || !m_BloomDescriptorSets[0] || m_BloomTexture->GetWidth() == 0 || m_BloomTexture->GetHeight() == 0)
        {
            LWARN("BloomPass early exit: shaderCompiled=%d descSet=%d texW=%u texH=%u compute=%d",
                  m_BloomPassShader->IsCompiled(), (bool)m_BloomDescriptorSets[0],
                  m_BloomTexture->GetWidth(), m_BloomTexture->GetHeight(), m_SupportCompute);
            return;
        }

        int descCount      = (int)m_BloomDescriptorSets.Size();
        int targetMipCount = m_BloomTexture->GetMipMapLevels();

        int targetDescCount = 1 + (targetMipCount - 2) * 2 + 1 + (targetMipCount - 2 - 3);

        if(descCount != targetDescCount)
        {
            m_BloomDescriptorSets.Clear();

            Graphics::DescriptorDesc descriptorDesc {};
            descriptorDesc.layoutIndex = 0;
            descriptorDesc.shader      = m_BloomPassShader.get();

            for(int i = 0; i < targetDescCount; i++)
            {
                m_BloomDescriptorSets.PushBack(SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc)));
            }
        }

        int passCount = 0;

        struct BloomComputePushConstants
        {
            Vec4 Params;
            Vec4 Params2; // float LOD = 0.0f;
            // int Mode = 0; // 0 = prefilter, 1 = downsample, 2 = firstUpsample, 3 = upsample
        } bloomComputePushConstants;

        bloomComputePushConstants.Params    = { m_CurrentScene->GetSettings().RenderSettings.BloomThreshold, m_CurrentScene->GetSettings().RenderSettings.BloomThreshold - m_CurrentScene->GetSettings().RenderSettings.BloomKnee, m_CurrentScene->GetSettings().RenderSettings.BloomKnee * 2.0f, 0.25f / m_CurrentScene->GetSettings().RenderSettings.BloomKnee };
        bloomComputePushConstants.Params2.x = (float)0;
        bloomComputePushConstants.Params2.y = (float)0;
        bloomComputePushConstants.Params2.z = (float)m_MainTexture->GetWidth();
        bloomComputePushConstants.Params2.w = (float)m_MainTexture->GetHeight();

        // Rounded up, like the mips below: rounding down leaves the last columns / rows of a size that is not a
        // multiple of 4 unwritten, and whatever was in that memory (NaN included) is then blurred over the whole
        // frame by the down / upsample chain - a black picture after tone mapping, on some runs and sizes only.
        uint32_t workGroupSize = 4;
        uint32_t workGroupsX   = (m_BloomTexture->GetWidth() + workGroupSize - 1) / workGroupSize;
        uint32_t workGroupsY   = (m_BloomTexture->GetHeight() + workGroupSize - 1) / workGroupSize;

        commandBuffer->UnBindPipeline();

        m_BloomDescriptorSets[0]->SetTexture(1, m_LastRenderTarget);
        m_BloomDescriptorSets[0]->SetTexture(2, m_LastRenderTarget);

        if(m_SupportCompute)
        {
            m_BloomDescriptorSets[0]->SetTexture(0, m_BloomTexture);
            m_BloomDescriptorSets[0]->TransitionImages(commandBuffer);
        }
        m_BloomDescriptorSets[0]->Update();

        Graphics::PipelineDesc pipelineDesc {};
        pipelineDesc.shader              = m_BloomPassShader;
        pipelineDesc.polygonMode         = Graphics::PolygonMode::FILL;
        pipelineDesc.cullMode            = Graphics::CullMode::BACK;
        pipelineDesc.transparencyEnabled = false;
        pipelineDesc.mipIndex            = 0;
        pipelineDesc.DebugName           = "Bloom-Prefilter";
        if(!m_SupportCompute)
            pipelineDesc.colourTargets[0] = m_BloomTexture;

        auto pipeline = Graphics::Pipeline::Get(pipelineDesc);
        commandBuffer->BindPipeline(pipeline);

        auto& pushConstants = m_BloomPassShader->GetPushConstants();
        memcpy(pushConstants[0].data, &bloomComputePushConstants, sizeof(BloomComputePushConstants));
        m_BloomPassShader->BindPushConstants(commandBuffer, pipeline.get());

        // Pre Filter
        {

            auto set = m_BloomDescriptorSets[passCount].get();
            Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);

            if(m_SupportCompute)
            {
                Renderer::GetRenderer()->Dispatch(commandBuffer, workGroupsX, workGroupsY, 1);
                commandBuffer->BufferMemoryBarrier(CommandBuffer::PipelineStage::COMPUTE_SHADER, CommandBuffer::PipelineStage::COMPUTE_SHADER);
            }
            else
            {
                Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);
            }

            passCount++;
        }

        commandBuffer->UnBindPipeline();

        bloomComputePushConstants.Params2.y = 1;
        uint32_t mips;

        if(m_BloomTexture->GetMipMapLevels() < 2)
            mips = 1;
        else
            mips = m_BloomTexture->GetMipMapLevels() - 2;

        for(int i = 1; i < (targetMipCount - 2) * 2; i++)
        {
            if(i % 2 == 0)
            {
                m_BloomDescriptorSets[i]->SetTexture(1, m_BloomTexture1);
                m_BloomDescriptorSets[i]->SetTexture(2, m_LastRenderTarget);
            }
            else
            {
                m_BloomDescriptorSets[i]->SetTexture(1, m_BloomTexture);
                m_BloomDescriptorSets[i]->SetTexture(2, m_LastRenderTarget);
            }

            m_BloomDescriptorSets[i]->Update();
        }

        //        m_BloomDescriptorSets[1]->SetTexture("u_Texture", m_BloomTexture);
        //        m_BloomDescriptorSets[1]->SetTexture("u_BloomTexture", m_MainTexture);
        //
        //        m_BloomDescriptorSets[2]->SetTexture("u_Texture", m_BloomTexture1);
        //        m_BloomDescriptorSets[2]->SetTexture("u_BloomTexture", m_MainTexture);

        // m_BloomDescriptorSets[2]->Update();

        for(uint32_t i = 1; i < mips; i++)
        {
            auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
            commandBuffer->UnBindPipeline();
            {
                pipelineDesc.mipIndex  = i;
                pipelineDesc.DebugName = "Bloom-Downsample"; // fmt::format("Bloom-Downsample{0}", i);
                if(!m_SupportCompute)
                    pipelineDesc.colourTargets[0] = m_BloomTexture1;
                bloomComputePushConstants.Params2.z = (float)m_BloomTexture1->GetWidth(i);
                bloomComputePushConstants.Params2.w = (float)m_BloomTexture1->GetHeight(i);

                if(m_SupportCompute)
                {
                    m_BloomDescriptorSets[passCount]->SetTexture(0, m_BloomTexture1, i);
                }

                m_BloomDescriptorSets[passCount]->TransitionImages(commandBuffer);
                m_BloomDescriptorSets[passCount]->Update();

                pipeline = Graphics::Pipeline::Get(pipelineDesc);
                commandBuffer->BindPipeline(pipeline);

                bloomComputePushConstants.Params2.x = i - 1.0f;

                auto& pushConstants = m_BloomPassShader->GetPushConstants();
                memcpy(pushConstants[0].data, &bloomComputePushConstants, sizeof(BloomComputePushConstants));
                m_BloomPassShader->BindPushConstants(commandBuffer, pipeline.get());

                auto set = m_BloomDescriptorSets[passCount].get();
                Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
                if(m_SupportCompute)
                {
                    workGroupsX = (uint32_t)Maths::Ceil((float)m_BloomTexture->GetWidth(i) / (float)workGroupSize);
                    workGroupsY = (uint32_t)Maths::Ceil((float)m_BloomTexture->GetHeight(i) / (float)workGroupSize);

                    Renderer::GetRenderer()->Dispatch(commandBuffer, workGroupsX, workGroupsY, 1);
                    commandBuffer->BufferMemoryBarrier(CommandBuffer::PipelineStage::COMPUTE_SHADER, CommandBuffer::PipelineStage::COMPUTE_SHADER);
                }
                else
                {
                    Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);
                }

                passCount++;
            }
            {
                auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
                commandBuffer->UnBindPipeline();

                if(m_SupportCompute)
                {
                    m_BloomDescriptorSets[passCount]->SetTexture(0, m_BloomTexture, i);
                    m_BloomDescriptorSets[passCount]->TransitionImages(commandBuffer);
                    m_BloomDescriptorSets[passCount]->Update();
                }
                else
                {
                    // m_BloomDescriptorSets[passCount]->SetTexture("u_Texture", m_BloomTexture, i);
                    m_BloomDescriptorSets[passCount]->TransitionImages(commandBuffer);
                    m_BloomDescriptorSets[passCount]->Update();
                }

                pipelineDesc.mipIndex = i;

                if(!m_SupportCompute)
                    pipelineDesc.colourTargets[0] = m_BloomTexture;
                bloomComputePushConstants.Params2.z = (float)m_BloomTexture1->GetWidth(i);
                bloomComputePushConstants.Params2.w = (float)m_BloomTexture1->GetHeight(i);

                pipeline = Graphics::Pipeline::Get(pipelineDesc);
                commandBuffer->BindPipeline(pipeline);

                bloomComputePushConstants.Params2.x = (float)i;

                memcpy(pushConstants[0].data, &bloomComputePushConstants, sizeof(BloomComputePushConstants));
                m_BloomPassShader->BindPushConstants(commandBuffer, pipeline.get());

                auto set = m_BloomDescriptorSets[passCount].get();
                Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
                if(m_SupportCompute)
                {
                    workGroupsX = (uint32_t)Maths::Ceil((float)m_BloomTexture->GetWidth(i) / (float)workGroupSize);
                    workGroupsY = (uint32_t)Maths::Ceil((float)m_BloomTexture->GetHeight(i) / (float)workGroupSize);

                    Renderer::GetRenderer()->Dispatch(commandBuffer, workGroupsX, workGroupsY, 1);
                    commandBuffer->BufferMemoryBarrier(CommandBuffer::PipelineStage::COMPUTE_SHADER, CommandBuffer::PipelineStage::COMPUTE_SHADER);
                }
                else
                {
                    Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);
                }

                passCount++;
            }
        }

        // First Upsample
        {
            auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
            commandBuffer->UnBindPipeline();

            m_BloomDescriptorSets[passCount]->SetTexture(1, m_BloomTexture);
            m_BloomDescriptorSets[passCount]->SetTexture(2, m_LastRenderTarget);

            if(m_SupportCompute)
            {
                m_BloomDescriptorSets[passCount]->SetTexture(0, m_BloomTexture2, mips - 2);
            }
            m_BloomDescriptorSets[passCount]->TransitionImages(commandBuffer);
            m_BloomDescriptorSets[passCount]->Update();

            bloomComputePushConstants.Params2.y = (float)2;
            bloomComputePushConstants.Params2.x -= 1.0f;
            bloomComputePushConstants.Params2.z = (float)m_BloomTexture2->GetWidth(mips - 2);
            bloomComputePushConstants.Params2.w = (float)m_BloomTexture2->GetHeight(mips - 2);

            pipelineDesc.mipIndex  = mips - 2;
            pipelineDesc.DebugName = "Bloom-Upsample"; // fmt::format("Bloom-Upsample{0}", mips - 2);

            if(!m_SupportCompute)
                pipelineDesc.colourTargets[0] = m_BloomTexture2;
            pipeline = Graphics::Pipeline::Get(pipelineDesc);
            commandBuffer->BindPipeline(pipeline);

            memcpy(pushConstants[0].data, &bloomComputePushConstants, sizeof(BloomComputePushConstants));
            m_BloomPassShader->BindPushConstants(commandBuffer, pipeline.get());

            auto set2 = m_BloomDescriptorSets[passCount].get();
            Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set2, 1);
            if(m_SupportCompute)
            {
                workGroupsX = (uint32_t)Maths::Ceil((float)m_BloomTexture->GetWidth(mips - 2) / (float)workGroupSize);
                workGroupsY = (uint32_t)Maths::Ceil((float)m_BloomTexture->GetHeight(mips - 2) / (float)workGroupSize);

                Renderer::GetRenderer()->Dispatch(commandBuffer, workGroupsX, workGroupsY, 1);
                commandBuffer->BufferMemoryBarrier(CommandBuffer::PipelineStage::COMPUTE_SHADER, CommandBuffer::PipelineStage::COMPUTE_SHADER);
            }
            else
            {
                Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);
            }

            passCount++;
        }

        int test = 1;
        for(int i = passCount; i < passCount + (targetMipCount - 2 - 3) + 1; i++)
        {
            auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
            commandBuffer->UnBindPipeline();

            if(test % 2 == 0)
            {
                m_BloomDescriptorSets[i]->SetTexture(1, m_BloomTexture);
                m_BloomDescriptorSets[i]->SetTexture(2, m_BloomTexture1);
            }
            else
            {
                m_BloomDescriptorSets[i]->SetTexture(1, m_BloomTexture);
                m_BloomDescriptorSets[i]->SetTexture(2, m_BloomTexture2);
            }
            m_BloomDescriptorSets[i]->Update();

            test++;
        }

        // Can't transition images to right layout if render to a colour attachment
        // Need to sample from m_BloomTexture2 first then switch between that and m_BloomTexture1
        bool evenMip = true;

        // Upsample
        for(int32_t mip = mips - 3; mip >= 0; mip--)
        {
            auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
            commandBuffer->UnBindPipeline();

            bloomComputePushConstants.Params2.y = (float)3;
            bloomComputePushConstants.Params2.x = (float)mip;
            bloomComputePushConstants.Params2.z = (float)m_BloomTexture2->GetWidth(mip);
            bloomComputePushConstants.Params2.w = (float)m_BloomTexture2->GetHeight(mip);

            pipelineDesc.mipIndex  = mip;
            pipelineDesc.DebugName = "Bloom-Upsample"; // fmt::format("Bloom-Upsample{0}", mip);

            if(evenMip)
            {
                if(m_SupportCompute)
                {
                    m_BloomDescriptorSets[passCount]->SetTexture(0, m_BloomTexture1, mip);
                    m_BloomDescriptorSets[passCount]->TransitionImages(commandBuffer);
                    m_BloomDescriptorSets[passCount]->Update();
                }
                else
                    m_BloomDescriptorSets[passCount]->TransitionImages(commandBuffer);
            }
            else
            {
                if(m_SupportCompute)
                {
                    m_BloomDescriptorSets[passCount]->SetTexture(0, m_BloomTexture2, mip);
                    m_BloomDescriptorSets[passCount]->TransitionImages(commandBuffer);
                    m_BloomDescriptorSets[passCount]->Update();
                }
                else
                    m_BloomDescriptorSets[passCount]->TransitionImages(commandBuffer);
            }

            if(!m_SupportCompute)
                pipelineDesc.colourTargets[0] = evenMip ? m_BloomTexture1 : m_BloomTexture2;
            auto set2 = m_BloomDescriptorSets[passCount].get(); // evenMip ? m_BloomDescriptorSets[passCount].get() : m_BloomDescriptorSets[5].get();
            set2->TransitionImages(commandBuffer);

            pipeline = Graphics::Pipeline::Get(pipelineDesc);
            commandBuffer->BindPipeline(pipeline);

            memcpy(pushConstants[0].data, &bloomComputePushConstants, sizeof(BloomComputePushConstants));
            m_BloomPassShader->BindPushConstants(commandBuffer, pipeline.get());

            Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set2, 1);
            if(m_SupportCompute)
            {
                workGroupsX = (uint32_t)Maths::Ceil((float)m_BloomTexture2->GetWidth(mip) / (float)workGroupSize);
                workGroupsY = (uint32_t)Maths::Ceil((float)m_BloomTexture2->GetHeight(mip) / (float)workGroupSize);

                Renderer::GetRenderer()->Dispatch(commandBuffer, workGroupsX, workGroupsY, 1);
                commandBuffer->BufferMemoryBarrier(CommandBuffer::PipelineStage::COMPUTE_SHADER, CommandBuffer::PipelineStage::COMPUTE_SHADER);
            }
            else
            {
                Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);
            }

            passCount++;

            m_BloomTextureLastRenderered = evenMip ? m_BloomTexture1 : m_BloomTexture2;
            evenMip                      = !evenMip;
        }
    }

    void SceneRenderer::FXAAPass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("FXAA Pass");

        if(!m_MainTexture || !m_FXAAShader->IsCompiled())
            return;

        auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
        commandBuffer->UnBindPipeline();

        m_FXAAPassDescriptorSet->SetTexture(1, m_LastRenderTarget);

        if(m_SupportCompute)
        {
            m_FXAAPassDescriptorSet->SetTexture(1, m_PostProcessTexture1);
            m_FXAAPassDescriptorSet->SetTexture(0, m_LastRenderTarget);
            m_FXAAPassDescriptorSet->TransitionImages(commandBuffer);
        }
        else
            m_FXAAPassDescriptorSet->SetTexture(1, m_LastRenderTarget);

        m_FXAAPassDescriptorSet->Update();

        Graphics::PipelineDesc pipelineDesc {};
        pipelineDesc.shader              = m_FXAAShader;
        pipelineDesc.transparencyEnabled = false;
        pipelineDesc.clearTargets        = false;
        pipelineDesc.DebugName           = "FXAA";

        if(!m_SupportCompute)
            pipelineDesc.colourTargets[0] = m_PostProcessTexture1;

        auto pipeline = Graphics::Pipeline::Get(pipelineDesc);
        commandBuffer->BindPipeline(pipeline);

        auto set = m_FXAAPassDescriptorSet.get();
        Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);

        if(m_SupportCompute)
        {
            uint32_t workGroupSize = 4;
            uint32_t workGroupsX   = m_PostProcessTexture1->GetWidth() / workGroupSize;
            uint32_t workGroupsY   = m_PostProcessTexture1->GetHeight() / workGroupSize;

            Renderer::GetRenderer()->Dispatch(commandBuffer, workGroupsX, workGroupsY, 1);
            commandBuffer->BufferMemoryBarrier(CommandBuffer::PipelineStage::COMPUTE_SHADER, CommandBuffer::PipelineStage::FRAGMENT_SHADER);
        }
        else
        {
            Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);
        }

        Swap(m_PostProcessTexture1, m_LastRenderTarget);
    }

    void SceneRenderer::SMAAPass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("SMAA Pass");

        if(!m_LastRenderTarget || !m_SMAAEdgesShader->IsCompiled() || !m_SMAAWeightsShader->IsCompiled() || !m_SMAABlendShader->IsCompiled())
            return;

        auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();

        // Pass 1: edge detection (colour -> edges)
        {
            commandBuffer->UnBindPipeline();
            m_SMAAEdgesDescriptorSet->SetTexture(1, m_LastRenderTarget);
            m_SMAAEdgesDescriptorSet->Update();

            Graphics::PipelineDesc pd {};
            pd.shader              = m_SMAAEdgesShader;
            pd.transparencyEnabled = false;
            pd.clearTargets        = true; // edges shader discards non-edges; clear to 0 first
            pd.colourTargets[0]    = m_SMAAEdgesTexture;
            pd.DebugName           = "SMAA-Edges";

            auto pipeline = Graphics::Pipeline::Get(pd);
            commandBuffer->BindPipeline(pipeline);
            auto set = m_SMAAEdgesDescriptorSet.get();
            Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
            Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);
            commandBuffer->UnBindPipeline();
        }

        // Pass 2: blend weight calculation (edges -> weights)
        {
            m_SMAAWeightsDescriptorSet->SetTexture(1, m_SMAAEdgesTexture);
            m_SMAAWeightsDescriptorSet->Update();

            Graphics::PipelineDesc pd {};
            pd.shader              = m_SMAAWeightsShader;
            pd.transparencyEnabled = false;
            pd.clearTargets        = true;
            pd.colourTargets[0]    = m_SMAAWeightsTexture;
            pd.DebugName           = "SMAA-Weights";

            auto pipeline = Graphics::Pipeline::Get(pd);
            commandBuffer->BindPipeline(pipeline);
            auto set = m_SMAAWeightsDescriptorSet.get();
            Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
            Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);
            commandBuffer->UnBindPipeline();
        }

        // Pass 3: neighbourhood blending (colour + weights -> result)
        {
            m_SMAABlendDescriptorSet->SetTexture(1, m_LastRenderTarget);
            m_SMAABlendDescriptorSet->SetTexture(2, m_SMAAWeightsTexture);
            m_SMAABlendDescriptorSet->Update();

            Graphics::PipelineDesc pd {};
            pd.shader              = m_SMAABlendShader;
            pd.transparencyEnabled = false;
            pd.clearTargets        = false;
            pd.colourTargets[0]    = m_PostProcessTexture1;
            pd.DebugName           = "SMAA-Blend";

            auto pipeline = Graphics::Pipeline::Get(pd);
            commandBuffer->BindPipeline(pipeline);
            auto set = m_SMAABlendDescriptorSet.get();
            Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
            Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);
            commandBuffer->UnBindPipeline();
        }

        Swap(m_PostProcessTexture1, m_LastRenderTarget);
    }

    void SceneRenderer::DebandingPass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("Debanding Pass");

        auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
        commandBuffer->UnBindPipeline();

        if(!m_LastRenderTarget || !m_DebandingShader->IsCompiled())
            return;

        m_DebandingPassDescriptorSet->SetTexture(1, m_LastRenderTarget);
        m_DebandingPassDescriptorSet->Update();

        Graphics::PipelineDesc pipelineDesc {};
        pipelineDesc.shader              = m_DebandingShader;
        pipelineDesc.transparencyEnabled = false;
        pipelineDesc.clearTargets        = true;
        pipelineDesc.colourTargets[0]    = m_PostProcessTexture1;
        pipelineDesc.DebugName           = "Debanding";

        auto pipeline = Graphics::Pipeline::Get(pipelineDesc);
        commandBuffer->BindPipeline(pipeline);

        auto set = m_DebandingPassDescriptorSet.get();
        Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
        Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);

        Swap(m_PostProcessTexture1, m_LastRenderTarget);
    }

    void SceneRenderer::FilmicGrainPass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("Filmic Grain Pass");

        if(!m_MainTexture || !m_FilmicGrainShader->IsCompiled())
            return;

        auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
        commandBuffer->UnBindPipeline();

        m_FilmicGrainPassDescriptorSet->SetTexture(1, m_LastRenderTarget);
        m_FilmicGrainPassDescriptorSet->Update();

        Graphics::PipelineDesc pipelineDesc {};
        pipelineDesc.shader              = m_FilmicGrainShader;
        pipelineDesc.transparencyEnabled = false;
        pipelineDesc.clearTargets        = true;
        pipelineDesc.colourTargets[0]    = m_PostProcessTexture1;
        pipelineDesc.DebugName           = "FilmicGrain";

        auto pipeline = Graphics::Pipeline::Get(pipelineDesc);
        commandBuffer->BindPipeline(pipeline);

        auto& pushConstants = m_FilmicGrainShader->GetPushConstants();
        if(!pushConstants.Empty())
        {
            auto& pushConstant = m_FilmicGrainShader->GetPushConstants()[0];
            float time         = (float)Engine::GetTimeStep().GetElapsedSeconds();
            float intensity    = 0.02f;
            pushConstant.SetValue("Time", (void*)&time);
            pushConstant.SetValue("Intensity", (void*)&intensity);
            m_FilmicGrainShader->BindPushConstants(commandBuffer, pipeline.get());
        }

        auto set = m_FilmicGrainPassDescriptorSet.get();
        Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
        Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);

        Swap(m_PostProcessTexture1, m_LastRenderTarget);
    }

    void SceneRenderer::OutlinePass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("Outline Pass");

        if(!m_LastRenderTarget || !m_OutlineShader || !m_OutlineShader->IsCompiled())
            return;

        auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
        commandBuffer->UnBindPipeline();

        // m_OutlinePassDescriptorSet->SetTexture(1, m_LastRenderTarget);
        // m_OutlinePassDescriptorSet->Update();

        Graphics::PipelineDesc pipelineDesc {};
        pipelineDesc.shader              = m_OutlineShader;
        pipelineDesc.transparencyEnabled = false;
        pipelineDesc.clearTargets        = true;
        pipelineDesc.colourTargets[0]    = m_PostProcessTexture1;
        pipelineDesc.DebugName           = "Outline";

        auto pipeline = Graphics::Pipeline::Get(pipelineDesc);
        commandBuffer->BindPipeline(pipeline);

        // auto set = m_OutlinePassDescriptorSet.get();
        // Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
        Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);

        Swap(m_PostProcessTexture1, m_LastRenderTarget);
    }

    void SceneRenderer::ChromaticAberationPass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("ChromaticAberation Pass");

        if(!m_Camera || !m_LastRenderTarget || !m_ChromaticAberationShader->IsCompiled())
            return;

        auto set      = m_ChromaticAberationPassDescriptorSet.get();
        float data[4] = { 100.0f, m_Camera->GetAperture(), 0.0f, 0.0f };
        set->SetTexture(1, m_LastRenderTarget);
        set->SetUniformBufferData(0, data);
        set->Update();

        Graphics::PipelineDesc pipelineDesc {};
        pipelineDesc.shader              = m_ChromaticAberationShader;
        pipelineDesc.transparencyEnabled = false;
        pipelineDesc.clearTargets        = true;
        pipelineDesc.colourTargets[0]    = m_PostProcessTexture1;
        pipelineDesc.DebugName           = "ChromaticAberation";

        auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
        auto pipeline      = Graphics::Pipeline::Get(pipelineDesc);
        commandBuffer->BindPipeline(pipeline);

        Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
        Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);

        Swap(m_PostProcessTexture1, m_LastRenderTarget);
    }

    void SceneRenderer::VignettePass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("Vignette Pass");

        if(!m_LastRenderTarget || !m_VignetteShader || !m_VignetteShader->IsCompiled() || !m_VignettePassDescriptorSet)
            return;

        const auto& renderSettings = m_CurrentScene->GetSettings().RenderSettings;
        auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
        commandBuffer->UnBindPipeline();

        m_VignettePassDescriptorSet->SetTexture(1, m_LastRenderTarget);
        m_VignettePassDescriptorSet->Update();

        Graphics::PipelineDesc pipelineDesc {};
        pipelineDesc.shader              = m_VignetteShader;
        pipelineDesc.transparencyEnabled = false;
        pipelineDesc.clearTargets        = true;
        pipelineDesc.colourTargets[0]    = m_PostProcessTexture1;
        pipelineDesc.DebugName           = "Vignette";

        auto pipeline = Graphics::Pipeline::Get(pipelineDesc);
        commandBuffer->BindPipeline(pipeline);

        struct VignettePC
        {
            float Colour[3];
            float Intensity;
            float Smoothness;
            float Roundness;
            float _p0;
            float _p1;
        } pc;
        pc.Colour[0] = renderSettings.VignetteColour.x;
        pc.Colour[1] = renderSettings.VignetteColour.y;
        pc.Colour[2] = renderSettings.VignetteColour.z;
        pc.Intensity = renderSettings.VignetteIntensity;
        pc.Smoothness = renderSettings.VignetteSmoothness;
        pc.Roundness  = renderSettings.VignetteRoundness;
        pc._p0 = 0.0f; pc._p1 = 0.0f;

        auto& pushConstants = m_VignetteShader->GetPushConstants();
        if(!pushConstants.Empty())
        {
            pushConstants[0].SetData(&pc);
            m_VignetteShader->BindPushConstants(commandBuffer, pipeline.get());
        }

        auto set = m_VignettePassDescriptorSet.get();
        Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
        Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);

        Swap(m_PostProcessTexture1, m_LastRenderTarget);
    }

    static void DMul4(const double* a, const double* b, double* out)
    {
        for(int c = 0; c < 4; c++)
            for(int r = 0; r < 4; r++)
            {
                double s = 0.0;
                for(int k = 0; k < 4; k++)
                    s += a[k * 4 + r] * b[c * 4 + k];
                out[c * 4 + r] = s;
            }
    }

    // Standard adjugate inverse (MESA gluInvertMatrix), column-major, double.
    static bool DInverse4(const double* m, double* invOut)
    {
        double inv[16];
        inv[0]  = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] + m[9] * m[7] * m[14] + m[13] * m[6] * m[11] - m[13] * m[7] * m[10];
        inv[4]  = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] - m[8] * m[7] * m[14] - m[12] * m[6] * m[11] + m[12] * m[7] * m[10];
        inv[8]  = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] + m[8] * m[7] * m[13] + m[12] * m[5] * m[11] - m[12] * m[7] * m[9];
        inv[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] - m[8] * m[6] * m[13] - m[12] * m[5] * m[10] + m[12] * m[6] * m[9];
        inv[1]  = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15] - m[9] * m[3] * m[14] - m[13] * m[2] * m[11] + m[13] * m[3] * m[10];
        inv[5]  = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15] + m[8] * m[3] * m[14] + m[12] * m[2] * m[11] - m[12] * m[3] * m[10];
        inv[9]  = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15] - m[8] * m[3] * m[13] - m[12] * m[1] * m[11] + m[12] * m[3] * m[9];
        inv[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14] + m[8] * m[2] * m[13] + m[12] * m[1] * m[10] - m[12] * m[2] * m[9];
        inv[2]  = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] - m[5] * m[2] * m[15] + m[5] * m[3] * m[14] + m[13] * m[2] * m[7] - m[13] * m[3] * m[6];
        inv[6]  = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] + m[4] * m[2] * m[15] - m[4] * m[3] * m[14] - m[12] * m[2] * m[7] + m[12] * m[3] * m[6];
        inv[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] - m[4] * m[1] * m[15] + m[4] * m[3] * m[13] + m[12] * m[1] * m[7] - m[12] * m[3] * m[5];
        inv[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] + m[4] * m[1] * m[14] - m[4] * m[2] * m[13] - m[12] * m[1] * m[6] + m[12] * m[2] * m[5];
        inv[3]  = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] + m[5] * m[2] * m[11] - m[5] * m[3] * m[10] - m[9] * m[2] * m[7] + m[9] * m[3] * m[6];
        inv[7]  = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] - m[4] * m[2] * m[11] + m[4] * m[3] * m[10] + m[8] * m[2] * m[7] - m[8] * m[3] * m[6];
        inv[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] + m[4] * m[1] * m[11] - m[4] * m[3] * m[9] - m[8] * m[1] * m[7] + m[8] * m[3] * m[5];
        inv[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] - m[4] * m[1] * m[10] + m[4] * m[2] * m[9] + m[8] * m[1] * m[6] - m[8] * m[2] * m[5];

        double det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
        if(det == 0.0)
            return false;
        det = 1.0 / det;
        for(int i = 0; i < 16; i++)
            invOut[i] = inv[i] * det;
        return true;
    }

    void SceneRenderer::MotionBlurPass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("Motion Blur Pass");

        if(!m_LastRenderTarget || !m_MotionBlurShader || !m_MotionBlurShader->IsCompiled() || !m_MotionBlurPassDescriptorSet)
            return;
        if(!m_Camera || !m_CameraTransform || !m_ForwardData.m_DepthTexture)
            return;

        const auto& renderSettings = m_CurrentScene->GetSettings().RenderSettings;

        const Mat4 camWorld = m_CameraTransform->GetWorldMatrix();
        const Mat4 currView = camWorld.Inverse();
        const Mat4 proj     = m_Camera->GetGPUProjectionMatrix();

        if(!m_HasPrevViewProj)
        {
            m_PrevViewProj    = currView; // stores the previous VIEW now
            m_HasPrevViewProj = true;
        }

        double dProj[16], dInvProj[16], dPrevView[16], dCamWorld[16];
        for(int i = 0; i < 16; i++)
        {
            dProj[i]     = (double)proj.values[i];
            dPrevView[i] = (double)m_PrevViewProj.values[i];
            dCamWorld[i] = (double)camWorld.values[i];
        }
        Mat4 reproj; // identity (zero velocity) fallback if proj is singular
        double t0[16], t1[16];
        if(DInverse4(dProj, dInvProj))
        {
            DMul4(dCamWorld, dInvProj, t0);
            DMul4(dPrevView, t0, t1);
            DMul4(dProj, t1, t0);
            for(int i = 0; i < 16; i++)
                reproj.values[i] = (float)t0[i];
        }

        auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
        commandBuffer->UnBindPipeline();

        struct MotionBlurUBO
        {
            Mat4 Reproj;
            float Strength;
            int SampleCount;
            float _p0;
            float _p1;
        } data;
        data.Reproj      = reproj;
        data.Strength    = renderSettings.MotionBlurStrength;
        data.SampleCount = renderSettings.MotionBlurSamples;
        if(data.SampleCount < 1) data.SampleCount = 1;
        if(data.SampleCount > 32) data.SampleCount = 32;
        data._p0 = 0.0f; data._p1 = 0.0f;

        m_MotionBlurPassDescriptorSet->SetUniformBufferData(0, &data);
        m_MotionBlurPassDescriptorSet->SetTexture(1, m_LastRenderTarget);
        m_MotionBlurPassDescriptorSet->SetTexture(2, m_ForwardData.m_DepthTexture);
        m_MotionBlurPassDescriptorSet->Update();

        Graphics::PipelineDesc pipelineDesc {};
        pipelineDesc.shader              = m_MotionBlurShader;
        pipelineDesc.transparencyEnabled = false;
        pipelineDesc.clearTargets        = true;
        pipelineDesc.colourTargets[0]    = m_PostProcessTexture1;
        pipelineDesc.DebugName           = "MotionBlur";

        auto pipeline = Graphics::Pipeline::Get(pipelineDesc);
        commandBuffer->BindPipeline(pipeline);

        auto set = m_MotionBlurPassDescriptorSet.get();
        Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
        Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);

        Swap(m_PostProcessTexture1, m_LastRenderTarget);

        m_PrevViewProj = currView;
    }

    void SceneRenderer::SSRPass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("SSR Pass");

        if(!m_LastRenderTarget || !m_SSRShader || !m_SSRShader->IsCompiled() || !m_SSRPassDescriptorSet)
            return;
        if(!m_Camera || !m_CameraTransform || !m_NormalTexture)
            return;

        const auto& renderSettings = m_CurrentScene->GetSettings().RenderSettings;

        Texture* gbuffer = m_MainTextureSamples > 1 ? (Texture*)m_NormalResolveTexture : (Texture*)m_NormalTexture;

        auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
        commandBuffer->UnBindPipeline();

        struct SSRUBO
        {
            Mat4 projection;
            Mat4 invProj;
            Mat4 view;
            float Near;
            float Far;
            float MaxDistance;
            float Thickness;
            int MaxSteps;
            int BinarySteps;
            float Strength;
            float MaxRoughness;
            float EnvMipCount;
            float EnvIntensity;
            float _pad0;
            float _pad1;
        } data;

        const bool hasEnv = renderSettings.SSREnvIntensity > 0.0f
            && m_ForwardData.m_EnvironmentMap
            && m_ForwardData.m_EnvironmentMap != m_DefaultTextureCube;

        data.projection   = m_Camera->GetGPUProjectionMatrix();
        data.invProj      = data.projection.Inverse();
        data.view         = m_CameraTransform->GetWorldMatrix().Inverse();
        data.Near         = m_Camera->GetNear();
        data.Far          = m_Camera->GetFar();
        data.MaxDistance  = renderSettings.SSRMaxDistance;
        data.Thickness    = renderSettings.SSRThickness;
        const QualitySettings& qs = Application::Get().GetQualitySettings();
        data.MaxSteps     = Maths::Clamp(renderSettings.SSRMaxSteps, 1, qs.SSRStepCap());
        data.BinarySteps  = Maths::Clamp(renderSettings.SSRBinarySteps, 0, qs.SSRBinaryStepCap());
        data.Strength     = renderSettings.SSRStrength;
        data.MaxRoughness = Maths::Max(renderSettings.SSRMaxRoughness, 0.01f);
        data.EnvMipCount  = hasEnv ? (float)m_ForwardData.m_EnvironmentMap->GetMipMapLevels() : 0.0f;
        data.EnvIntensity = renderSettings.SSREnvIntensity * m_Exposure * kSkyUnit; // the maps are in sky units, as the skybox / IBL draw them
        data._pad0        = 0.0f;
        data._pad1        = 0.0f;

        m_SSRPassDescriptorSet->SetUniformBufferData(0, &data);
        m_SSRPassDescriptorSet->SetTexture(1, m_LastRenderTarget);
        m_SSRPassDescriptorSet->SetTexture(2, gbuffer);
        // Always bind a valid cube (default when no env) so the layout is satisfied.
        m_SSRPassDescriptorSet->SetTexture(3, m_ForwardData.m_EnvironmentMap ? m_ForwardData.m_EnvironmentMap : m_DefaultTextureCube, 0, TextureType::CUBE);
        m_SSRPassDescriptorSet->TransitionImages(commandBuffer);
        m_SSRPassDescriptorSet->Update();

        Graphics::PipelineDesc pipelineDesc {};
        pipelineDesc.shader              = m_SSRShader;
        pipelineDesc.transparencyEnabled = false;
        pipelineDesc.clearTargets        = true;
        pipelineDesc.colourTargets[0]    = m_PostProcessTexture1;
        pipelineDesc.DebugName           = "SSR";

        auto pipeline = Graphics::Pipeline::Get(pipelineDesc);
        commandBuffer->BindPipeline(pipeline);

        auto set = m_SSRPassDescriptorSet.get();
        Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
        Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);

        Swap(m_PostProcessTexture1, m_LastRenderTarget);
    }

    void SceneRenderer::ContactShadowPass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("Contact Shadow Pass");

        if(!m_Camera || !m_CameraTransform || !m_ContactShadowDescriptorSet || !m_HasSun)
            return;

        const auto& rs     = m_CurrentScene->GetSettings().RenderSettings;
        auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
        commandBuffer->UnBindPipeline();

        const Mat4 view = m_CameraTransform->GetWorldMatrix().Inverse();
        Vec3 lightView  = Vec3(view * Vec4(m_SunDirection, 0.0f));
        lightView.Normalise();

        struct ContactUBO
        {
            Mat4 Projection;
            Mat4 InvProjection;
            Vec4 LightDirection;
            Vec4 Params;
        } data;
        data.Projection     = m_Camera->GetGPUProjectionMatrix();
        data.InvProjection  = data.Projection.Inverse();
        data.LightDirection = Vec4(lightView, 1.0f);
        // a new noise offset per frame only helps when TAA averages it
        data.Params = Vec4(rs.ContactShadowLength, rs.ContactShadowThickness, (float)Maths::Clamp(rs.ContactShadowSteps, 4, 64), m_TAAJittered ? (float)(m_TAAFrame % 64) : 0.0f);

        m_ContactShadowDescriptorSet->SetUniformBufferData(0, &data);
        m_ContactShadowDescriptorSet->SetTexture(1, m_ForwardData.m_DepthTexture);
        m_ContactShadowDescriptorSet->TransitionImages(commandBuffer);
        m_ContactShadowDescriptorSet->Update();

        Graphics::PipelineDesc pipelineDesc {};
        pipelineDesc.shader              = m_ContactShadowShader;
        pipelineDesc.transparencyEnabled = false;
        pipelineDesc.clearTargets        = true;
        pipelineDesc.colourTargets[0]    = m_ContactShadowTexture;
        pipelineDesc.DebugName           = "ContactShadows";

        auto pipeline = Graphics::Pipeline::Get(pipelineDesc);
        commandBuffer->BindPipeline(pipeline);
        auto set = m_ContactShadowDescriptorSet.get();
        Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
        Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);
        commandBuffer->UnBindPipeline();

        m_ContactShadowsValid = true;
    }

    void SceneRenderer::GIPass()
    {
        LUMOS_PROFILE_FUNCTION();
        const auto& rs = m_CurrentScene->GetSettings().RenderSettings;

        ProbeGIParams params;
        params.Spacing      = rs.GIProbeSpacing;
        params.Hysteresis   = rs.GIHysteresis;
        params.Bounce       = rs.GIBounce;
        params.UpdateStride = (u32)Maths::Max(1, rs.GIUpdateStride);
        params.LightScale    = 1.0f / kSkyUnit;
        params.EmissiveScale = 1.0f / Maths::Max(m_Exposure * kSkyUnit, 1e-6f);
        params.LightRange    = rs.LightRange;

        // the sky as the skybox has it, whatever probe the frame is lit by
        Texture* sky = m_CubeMap ? m_CubeMap : (m_ForwardData.m_EnvironmentMap ? m_ForwardData.m_EnvironmentMap : m_DefaultTextureCube);
        m_GI->Update(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), sky, m_SunDirection, m_SunIlluminance / kSkyUnit, m_HasSun, params);
    }

    void SceneRenderer::VolumetricFogPass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("Volumetric Fog Pass");

        if(!m_LastRenderTarget || !m_Camera || !m_CameraTransform)
            return;

        const auto& rs = m_CurrentScene->GetSettings().RenderSettings;
        if(rs.VolumetricFogDensity <= 0.0f)
            return;

        if(!m_FogAllocated)
        {
            m_FogAllocated = true;
            m_FogTexture->Resize(Maths::Max(1u, m_TargetWidth / 2), Maths::Max(1u, m_TargetHeight / 2));
            m_FogDepthTexture->Resize(Maths::Max(1u, m_TargetWidth / 2), Maths::Max(1u, m_TargetHeight / 2));
        }

        auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
        commandBuffer->UnBindPipeline();

        const Mat4 invViewProj = m_TAAViewProj.Inverse();
        const Vec3 eye         = m_CameraTransform->GetWorldPosition();
        const float maxDist    = Maths::Max(1.0f, Maths::Min(rs.VolumetricFogDistance, m_Camera->GetFar()));
        const bool shadows     = m_ShadowsAllocated && m_HasSun && rs.ShadowsEnabled && Application::Get().GetQualitySettings().EnableShadows;

        struct FogUBO
        {
            Mat4 InvViewProj;
            Mat4 View;
            Mat4 ShadowTransform[SHADOWMAP_MAX];
            Mat4 BiasMatrix;
            Vec4 CameraPosition;
            Vec4 SunDirection;
            Vec4 SunColour;
            Vec4 FogColour;
            Vec4 FogParams;
            Vec4 Params;
            Vec4 SplitDepths;
            Vec4 Misc;
        } data;
        data.InvViewProj = invViewProj;
        data.View        = m_CameraTransform->GetWorldMatrix().Inverse();
        MemoryCopy(data.ShadowTransform, m_ShadowData.m_ShadowProjView, sizeof(Mat4) * SHADOWMAP_MAX);
        data.BiasMatrix     = m_ForwardData.m_BiasMatrix;
        data.CameraPosition = Vec4(eye, maxDist);
        data.SunDirection   = Vec4(m_SunDirection, shadows ? 1.0f : 0.0f);
        data.SunColour      = Vec4(m_SunIlluminance * m_Exposure, (float)m_ShadowData.m_ShadowMapNum);
        data.FogColour      = Vec4(rs.VolumetricFogColour, 1.0f);
        data.FogParams      = Vec4(rs.VolumetricFogDensity, rs.VolumetricFogFalloff, rs.VolumetricFogBaseHeight, rs.VolumetricFogAmbient);
        data.Params         = Vec4(Maths::Clamp(rs.VolumetricFogAnisotropy, -0.95f, 0.95f), (float)Maths::Clamp(rs.VolumetricFogSteps, 4, 128), m_TAAJittered ? (float)(m_TAAFrame % 64) : 0.0f, rs.VolumetricFogSun);
        data.SplitDepths    = m_ShadowData.m_SplitDepth;
        data.Misc           = Vec4(m_Exposure * kSkyUnit, m_HasSun ? 1.0f : 0.0f, 0.0f, 0.0f);

        Texture* irradiance = m_ForwardData.m_IrradianceMap ? m_ForwardData.m_IrradianceMap : m_DefaultTextureCube;
        m_VolumetricFogDescriptorSet->SetUniformBufferData(0, &data);
        m_VolumetricFogDescriptorSet->SetTexture(1, m_ForwardData.m_DepthTexture);
        m_VolumetricFogDescriptorSet->SetTexture(2, reinterpret_cast<Texture*>(m_ShadowData.m_ShadowTex), 0, TextureType::DEPTHARRAY);
        m_VolumetricFogDescriptorSet->SetTexture(3, irradiance, 0, TextureType::CUBE);
        m_VolumetricFogDescriptorSet->TransitionImages(commandBuffer);
        m_VolumetricFogDescriptorSet->Update();

        Graphics::PipelineDesc pipelineDesc {};
        pipelineDesc.shader              = m_VolumetricFogShader;
        pipelineDesc.transparencyEnabled = false;
        pipelineDesc.clearTargets        = true;
        pipelineDesc.colourTargets[0]    = m_FogTexture;
        pipelineDesc.colourTargets[1]    = m_FogDepthTexture;
        pipelineDesc.DebugName           = "VolumetricFog";

        auto pipeline = Graphics::Pipeline::Get(pipelineDesc);
        commandBuffer->BindPipeline(pipeline);
        auto set = m_VolumetricFogDescriptorSet.get();
        Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
        Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);
        commandBuffer->UnBindPipeline();

        // full res: blend it over the picture, fog texels picked by depth
        struct ApplyUBO
        {
            Mat4 InvViewProj;
            Vec4 CameraPosition;
        } apply;
        apply.InvViewProj    = invViewProj;
        apply.CameraPosition = Vec4(eye, maxDist);

        m_VolumetricFogApplyDescriptorSet->SetUniformBufferData(0, &apply);
        m_VolumetricFogApplyDescriptorSet->SetTexture(1, m_LastRenderTarget);
        m_VolumetricFogApplyDescriptorSet->SetTexture(2, m_FogTexture);
        m_VolumetricFogApplyDescriptorSet->SetTexture(3, m_FogDepthTexture);
        m_VolumetricFogApplyDescriptorSet->SetTexture(4, m_ForwardData.m_DepthTexture);
        m_VolumetricFogApplyDescriptorSet->TransitionImages(commandBuffer);
        m_VolumetricFogApplyDescriptorSet->Update();

        Graphics::PipelineDesc applyDesc {};
        applyDesc.shader              = m_VolumetricFogApplyShader;
        applyDesc.transparencyEnabled = false;
        applyDesc.clearTargets        = true;
        applyDesc.colourTargets[0]    = m_PostProcessTexture1;
        applyDesc.DebugName           = "VolumetricFogApply";

        auto applyPipeline = Graphics::Pipeline::Get(applyDesc);
        commandBuffer->BindPipeline(applyPipeline);
        set = m_VolumetricFogApplyDescriptorSet.get();
        Renderer::BindDescriptorSets(applyPipeline.get(), commandBuffer, 0, &set, 1);
        Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);

        Swap(m_PostProcessTexture1, m_LastRenderTarget);
    }

    void SceneRenderer::TAAPass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("TAA Pass");

        if(!m_LastRenderTarget || !m_Camera || !m_CameraTransform || !m_TAADescriptorSet)
            return;

        EnsureTAATargets();
        const auto& rs = m_CurrentScene->GetSettings().RenderSettings;

        // a cut (teleport, camera switch) leaves nothing to reproject from
        const Vec3 eye = m_CameraTransform->GetWorldPosition();
        if(Maths::Length(eye - m_TAAPrevCameraPos) > 25.0f)
            m_TAAHistoryValid = false;

        double dViewProj[16], dInv[16], dPrev[16], dOut[16];
        for(int i = 0; i < 16; i++)
        {
            dViewProj[i] = (double)m_TAAViewProj.values[i];
            dPrev[i]     = (double)m_TAAPrevViewProj.values[i];
        }
        Mat4 reproj; // identity if the current view-projection can't be inverted
        if(DInverse4(dViewProj, dInv))
        {
            DMul4(dPrev, dInv, dOut);
            for(int i = 0; i < 16; i++)
                reproj.values[i] = (float)dOut[i];
        }
        else
            m_TAAHistoryValid = false;

        Texture2D* history = m_TAAHistory[m_TAAHistoryIndex];
        Texture2D* next    = m_TAAHistory[m_TAAHistoryIndex ^ 1];

        struct TAAUBO
        {
            Mat4 Reproj;
            Vec4 Jitter;
            Vec4 Params;
        } data;
        data.Reproj = reproj;
        data.Jitter = Vec4(m_TAAJitter.x, m_TAAJitter.y, 1.0f / (float)Maths::Max(1u, m_LastRenderTarget->GetWidth()), 1.0f / (float)Maths::Max(1u, m_LastRenderTarget->GetHeight()));
        data.Params = Vec4(Maths::Clamp(rs.TAAHistoryWeight, 0.0f, 0.98f), Maths::Clamp(rs.TAAMotionWeight, 0.0f, 0.98f), m_TAAHistoryValid ? 0.0f : 1.0f, Maths::Max(0.5f, rs.TAAClipGamma));

        auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
        commandBuffer->UnBindPipeline();

        m_TAADescriptorSet->SetUniformBufferData(0, &data);
        m_TAADescriptorSet->SetTexture(1, m_LastRenderTarget);
        m_TAADescriptorSet->SetTexture(2, history);
        m_TAADescriptorSet->SetTexture(3, m_ForwardData.m_DepthTexture);
        m_TAADescriptorSet->TransitionImages(commandBuffer);
        m_TAADescriptorSet->Update();

        Graphics::PipelineDesc pipelineDesc {};
        pipelineDesc.shader              = m_TAAShader;
        pipelineDesc.transparencyEnabled = false;
        pipelineDesc.clearTargets        = false;
        pipelineDesc.colourTargets[0]    = m_PostProcessTexture1;
        pipelineDesc.colourTargets[1]    = next;
        pipelineDesc.DebugName           = "TAA";

        auto pipeline = Graphics::Pipeline::Get(pipelineDesc);
        commandBuffer->BindPipeline(pipeline);
        auto set = m_TAADescriptorSet.get();
        Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
        Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);
        commandBuffer->UnBindPipeline();

        Swap(m_PostProcessTexture1, m_LastRenderTarget);
        m_TAAHistoryIndex ^= 1;
        m_TAAHistoryValid  = true;
        m_TAAPrevViewProj  = m_TAAViewProj;
        m_TAAPrevCameraPos = eye;
    }

    void SceneRenderer::EyeAdaptationPass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("Eye Adaptation Pass");

        const auto& settings = m_CurrentScene->GetSettings().RenderSettings;
        if(!settings.AdaptiveExposure || !m_AdaptiveExposureReady || !m_LastRenderTarget)
            return;

        auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
        commandBuffer->UnBindPipeline();

        const uint32_t texWidth  = m_LastRenderTarget->GetWidth();
        const uint32_t texHeight = m_LastRenderTarget->GetHeight();
        const float minLog       = settings.AdaptiveMinLogLum;
        const float maxLog       = settings.AdaptiveMaxLogLum;
        const float logRange     = (maxLog > minLog) ? (maxLog - minLog) : 1.0f;

        // ---- Histogram pass ----
        m_LuminanceHistogramDescriptorSet->SetTexture(0, m_LastRenderTarget);
        m_LuminanceHistogramDescriptorSet->SetStorageBuffer(1, m_LuminanceHistogramSSBO);
        m_LuminanceHistogramDescriptorSet->Update();
        // Transition the HDR target to a layout the compute shader can sample.
        m_LuminanceHistogramDescriptorSet->TransitionImages(commandBuffer);

        {
            Graphics::PipelineDesc pipelineDesc {};
            pipelineDesc.shader    = m_LuminanceHistogramShader;
            pipelineDesc.DebugName = "LuminanceHistogram";
            auto pipeline          = Graphics::Pipeline::Get(pipelineDesc);
            commandBuffer->BindPipeline(pipeline);

            struct HistogramPC
            {
                float MinLogLum;
                float InvLogLumRange;
                uint32_t TexWidth;
                uint32_t TexHeight;
            } pc;
            pc.MinLogLum      = minLog;
            pc.InvLogLumRange = 1.0f / logRange;
            pc.TexWidth       = texWidth;
            pc.TexHeight      = texHeight;

            auto& pushConstants = m_LuminanceHistogramShader->GetPushConstants()[0];
            pushConstants.SetData(&pc);
            m_LuminanceHistogramShader->BindPushConstants(commandBuffer, pipeline.get());

            auto set = m_LuminanceHistogramDescriptorSet.get();
            Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
            const uint32_t groupX = (texWidth  + 15) / 16;
            const uint32_t groupY = (texHeight + 15) / 16;
            Renderer::GetRenderer()->Dispatch(commandBuffer, groupX, groupY, 1);
        }

        // Make histogram writes visible to the reduction pass.
        commandBuffer->BufferMemoryBarrier(
            CommandBuffer::PipelineStage::COMPUTE_SHADER,
            CommandBuffer::PipelineStage::COMPUTE_SHADER);

        // ---- Average / reduction pass ----
        m_LuminanceAverageDescriptorSet->SetStorageBuffer(0, m_LuminanceHistogramSSBO);
        m_LuminanceAverageDescriptorSet->SetStorageBuffer(1, m_AverageLuminanceSSBO);
        m_LuminanceAverageDescriptorSet->Update();

        {
            Graphics::PipelineDesc pipelineDesc {};
            pipelineDesc.shader    = m_LuminanceAverageShader;
            pipelineDesc.DebugName = "LuminanceAverage";
            auto pipeline          = Graphics::Pipeline::Get(pipelineDesc);
            commandBuffer->BindPipeline(pipeline);

            const float dt        = (float)Engine::GetTimeStep().GetSeconds();
            const float clampedDt = (dt > 0.25f) ? 0.25f : dt;

            struct AveragePC
            {
                float MinLogLum;
                float LogLumRange;
                float DeltaTime;
                float SpeedUp;
                float SpeedDown;
                uint32_t NumPixels;
            } pc;
            pc.MinLogLum   = minLog;
            pc.LogLumRange = logRange;
            pc.DeltaTime   = clampedDt;
            pc.SpeedUp     = settings.AdaptiveSpeedUp;
            pc.SpeedDown   = settings.AdaptiveSpeedDown;
            pc.NumPixels   = texWidth * texHeight;

            auto& pushConstants = m_LuminanceAverageShader->GetPushConstants()[0];
            pushConstants.SetData(&pc);
            m_LuminanceAverageShader->BindPushConstants(commandBuffer, pipeline.get());

            auto set = m_LuminanceAverageDescriptorSet.get();
            Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
            Renderer::GetRenderer()->Dispatch(commandBuffer, 1, 1, 1);
        }

        // Make the smoothed average visible to ToneMapping's fragment read.
        commandBuffer->BufferMemoryBarrier(
            CommandBuffer::PipelineStage::COMPUTE_SHADER,
            CommandBuffer::PipelineStage::FRAGMENT_SHADER);
    }

    float SceneRenderer::SubmitTexture(Texture* texture)
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        float result = 0.0f;
        bool found   = false;

        for(uint32_t i = 0; i < m_Renderer2DData.m_TextureCount; i++)
        {
            if(m_Renderer2DData.m_Textures[i] == texture)
            {
                result = static_cast<float>(i + 1);
                found  = true;
                break;
            }
        }

        if(!found)
        {
            if(m_Renderer2DData.m_TextureCount >= m_Renderer2DData.m_Limits.MaxTextures)
            {
                Render2DFlush();
                // UI (and camera-less scenes) batch in screen space - there may be no scene camera at all
                if(m_UIProjectionSize.x > 0.0f || !m_Camera || !m_CameraTransform)
                    Begin2DPass();
                else
                {
                    Renderer2DBeginBatch();
                    auto projView = m_Camera->GetGPUProjectionMatrix() * m_CameraTransform->GetWorldMatrix().Inverse();
                    m_Renderer2DData.m_DescriptorSet[m_Renderer2DData.m_BatchDrawCallIndex][0]->SetUniformBufferData(0, &projView);
                    m_Renderer2DData.m_DescriptorSet[m_Renderer2DData.m_BatchDrawCallIndex][0]->Update();
                }
            }
            m_Renderer2DData.m_Textures[m_Renderer2DData.m_TextureCount] = texture;
            m_Renderer2DData.m_TextureCount++;
            result = static_cast<float>(m_Renderer2DData.m_TextureCount);
        }
        return result;
    }

    void SceneRenderer::Render2DPass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("Render2D Pass");

        if(m_Renderer2DData.m_CommandQueue2D.Empty())
            return;

        Graphics::PipelineDesc pipelineDesc;
        pipelineDesc.shader              = m_Renderer2DData.m_Shader;
        pipelineDesc.polygonMode         = Graphics::PolygonMode::FILL;
        pipelineDesc.cullMode            = Graphics::CullMode::NONE;
        pipelineDesc.transparencyEnabled = true;
        pipelineDesc.blendMode           = BlendMode::SrcAlphaOneMinusSrcAlpha;
        pipelineDesc.clearTargets        = false;
        pipelineDesc.depthTarget         = reinterpret_cast<Texture*>(m_ForwardData.m_DepthTexture);
        pipelineDesc.colourTargets[0]    = m_MainTexture;
        // Blended sprites must not write depth: a transparent quad that writes depth
        // punches a hole in everything drawn after it, and coplanar quads z-fight.
        // Order comes from the back-to-front sort instead.
        pipelineDesc.DepthWrite          = false;
        pipelineDesc.DebugName           = "2D";
        pipelineDesc.samples             = m_MainTextureSamples;
        if(m_MainTextureSamples > 1)
            pipelineDesc.resolveTexture = m_ResolveTexture;

        m_Renderer2DData.m_Pipeline = Graphics::Pipeline::Get(pipelineDesc);

        Renderer2DBeginBatch();

        auto projView = m_Camera->GetGPUProjectionMatrix() * m_CameraTransform->GetWorldMatrix().Inverse();
        m_Renderer2DData.m_DescriptorSet[0][0]->SetUniformBufferData(0, &projView);
        m_Renderer2DData.m_DescriptorSet[0][0]->Update();

        for(auto& command : m_Renderer2DData.m_CommandQueue2D)
        {
            m_Stats.NumRenderedObjects++;

            if(m_Renderer2DData.m_IndexCount >= m_Renderer2DData.m_Limits.IndiciesSize)
            {
                Render2DFlush();
                Renderer2DBeginBatch();

                projView = m_Camera->GetGPUProjectionMatrix() * m_CameraTransform->GetWorldMatrix().Inverse();
                m_Renderer2DData.m_DescriptorSet[m_Renderer2DData.m_BatchDrawCallIndex][0]->SetUniformBufferData(0, &projView);
                m_Renderer2DData.m_DescriptorSet[m_Renderer2DData.m_BatchDrawCallIndex][0]->Update();
            }

            auto& renderable = command.renderable;
            auto& transform  = command.transform;

            const Vec2 min = renderable->GetPosition();
            const Vec2 max = renderable->GetPosition() + renderable->GetScale();

            const Vec4 colour = renderable->GetColour();
            const auto& uv    = renderable->GetUVs();
            Texture* texture  = renderable->GetTexture();

            float textureSlot = 0.0f;
            if(texture)
                textureSlot = SubmitTexture(texture);

            Vec3 vertex                       = transform * Vec4(min.x, min.y, 0.0f, 1.0f);
            m_Renderer2DData.m_Buffer->vertex = vertex;
            m_Renderer2DData.m_Buffer->uv     = Vec4(uv[0].x, uv[0].y, 0.0f, 0.0f);
            m_Renderer2DData.m_Buffer->tid    = Vec2(textureSlot, 0.0f);
            m_Renderer2DData.m_Buffer->colour = colour;
            m_Renderer2DData.m_Buffer++;

            vertex                            = transform * Vec4(max.x, min.y, 0.0f, 1.0f);
            m_Renderer2DData.m_Buffer->vertex = vertex;
            m_Renderer2DData.m_Buffer->uv     = Vec4(uv[1].x, uv[1].y, 0.0f, 0.0f);
            m_Renderer2DData.m_Buffer->tid    = Vec2(textureSlot, 0.0f);
            m_Renderer2DData.m_Buffer->colour = colour;
            m_Renderer2DData.m_Buffer++;

            vertex                            = transform * Vec4(max.x, max.y, 0.0f, 1.0f);
            m_Renderer2DData.m_Buffer->vertex = vertex;
            m_Renderer2DData.m_Buffer->uv     = Vec4(uv[2].x, uv[2].y, 0.0f, 0.0f);
            m_Renderer2DData.m_Buffer->tid    = Vec2(textureSlot, 0.0f);
            m_Renderer2DData.m_Buffer->colour = colour;
            m_Renderer2DData.m_Buffer++;

            vertex                            = transform * Vec4(min.x, max.y, 0.0f, 1.0f);
            m_Renderer2DData.m_Buffer->vertex = vertex;
            m_Renderer2DData.m_Buffer->uv     = Vec4(uv[3].x, uv[3].y, 0.0f, 0.0f);
            m_Renderer2DData.m_Buffer->tid    = Vec2(textureSlot, 0.0f);
            m_Renderer2DData.m_Buffer->colour = colour;
            m_Renderer2DData.m_Buffer++;

            m_Renderer2DData.m_IndexCount += 6;
        }

        if(m_Renderer2DData.m_IndexCount == 0)
        {
            return;
        }

        Render2DFlush();
    }

    void SceneRenderer::Renderer2DBeginBatch()
    {
        uint32_t currentFrame = Renderer::GetMainSwapChain()->GetCurrentBufferIndex();

        m_Renderer2DData.m_IndexCount   = 0;
        m_Renderer2DData.m_TextureCount = 0;

        if((int)m_Renderer2DData.m_VertexBuffers[currentFrame].Size() - 1 < (int)m_Renderer2DData.m_BatchDrawCallIndex)
        {
            auto& vertexBuffer = m_Renderer2DData.m_VertexBuffers[currentFrame].EmplaceBack(Graphics::VertexBuffer::Create(BufferUsage::DYNAMIC));
            vertexBuffer->Resize(BATCH_BUFFER_INITIAL);
        }

        m_Renderer2DData.m_Buffer = m_2DBufferBase[currentFrame][0];

        float projWidth   = m_UIProjectionSize.x > 0.0f ? m_UIProjectionSize.x : (float)m_MainTexture->GetWidth();
        float projHeight  = m_UIProjectionSize.y > 0.0f ? m_UIProjectionSize.y : (float)m_MainTexture->GetHeight();
        auto projView     = ReverseZ(Mat4::Orthographic(0.0f, projWidth, 0.0f, projHeight, -1.0f, 1.0f));
        float aspectRatio = projWidth / projHeight;

        if(m_Renderer2DData.m_DescriptorSet[m_Renderer2DData.m_BatchDrawCallIndex][0] == nullptr)
        {
            Graphics::DescriptorDesc descriptorDesc {};
            descriptorDesc.layoutIndex                                                 = 0;
            descriptorDesc.shader                                                      = m_Renderer2DData.m_Shader.get();
            m_Renderer2DData.m_DescriptorSet[m_Renderer2DData.m_BatchDrawCallIndex][0] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
        }

        m_Renderer2DData.m_DescriptorSet[m_Renderer2DData.m_BatchDrawCallIndex][0]->SetUniformBufferData(0, &projView);
        m_Renderer2DData.m_DescriptorSet[m_Renderer2DData.m_BatchDrawCallIndex][0]->Update();
    }

    void SceneRenderer::Render2DFlush()
    {
        LUMOS_PROFILE_FUNCTION();
        if(m_Renderer2DData.m_BatchDrawCallIndex >= m_Renderer2DData.m_Limits.MaxBatchDrawCalls)
        {
            static u64 s_LastWarnFrame = ~0ull;
            if(s_LastWarnFrame != Engine::Get().Statistics().FramesPerSecond)
            {
                s_LastWarnFrame = Engine::Get().Statistics().FramesPerSecond;
                LWARN("2D batch draw call limit (%u) hit - dropping draws", m_Renderer2DData.m_Limits.MaxBatchDrawCalls);
            }
            // same as the text flush: rewind the write pointer with the count
            m_Renderer2DData.m_Buffer       = m_2DBufferBase[Renderer::GetMainSwapChain()->GetCurrentBufferIndex()][0];
            m_Renderer2DData.m_IndexCount   = 0;
            m_Renderer2DData.m_TextureCount = 0;
            return;
        }

        uint32_t currentFrame                  = Renderer::GetMainSwapChain()->GetCurrentBufferIndex();
        Graphics::CommandBuffer* commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
        // keep the render pass open between batches: ending it reloads / stores the whole target

        uint32_t dataSize = (uint32_t)((uint8_t*)m_Renderer2DData.m_Buffer - (uint8_t*)m_2DBufferBase[currentFrame][0]);
        UploadBatch(m_Renderer2DData.m_VertexBuffers[currentFrame][m_Renderer2DData.m_BatchDrawCallIndex], dataSize, m_2DBufferBase[currentFrame][0]);
        commandBuffer->BindPipeline(m_Renderer2DData.m_Pipeline);
        m_Renderer2DData.m_VertexBuffers[currentFrame][m_Renderer2DData.m_BatchDrawCallIndex]->Bind(commandBuffer, m_Renderer2DData.m_Pipeline.get());

        if(m_Renderer2DData.m_DescriptorSet[m_Renderer2DData.m_BatchDrawCallIndex][1] == nullptr /*|| m_Renderer2DData.m_TextureCount != m_Renderer2DData.m_PreviousFrameTextureCount[m_Renderer2DData.m_BatchDrawCallIndex]*/)
        {
            /*
             || m_Renderer2DData.m_TextureCount != m_Renderer2DData.m_PreviousFrameTextureCount[m_Renderer2DData.m_BatchDrawCallIndex])
             When previous frame texture count was less then than the previous frame
             and the texture previously used was deleted, there was a crash - maybe moltenvk only
             May not be needed anymore
            */
            Graphics::DescriptorDesc descriptorDesc {};
            descriptorDesc.layoutIndex                                                 = 1;
            descriptorDesc.shader                                                      = m_Renderer2DData.m_Shader.get();
            m_Renderer2DData.m_DescriptorSet[m_Renderer2DData.m_BatchDrawCallIndex][1] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
        }

        for(int i = m_Renderer2DData.m_TextureCount; i < 16; i++)
        {
            m_Renderer2DData.m_Textures[i] = Material::GetDefaultTexture();
        }

        m_Renderer2DData.m_TextureCount = 16;

        if(m_Renderer2DData.m_TextureCount > 1)
            m_Renderer2DData.m_DescriptorSet[m_Renderer2DData.m_BatchDrawCallIndex][1]->SetTexture(0, m_Renderer2DData.m_Textures, m_Renderer2DData.m_TextureCount);
        else if(m_Renderer2DData.m_TextureCount == 0)
            m_Renderer2DData.m_DescriptorSet[m_Renderer2DData.m_BatchDrawCallIndex][1]->SetTexture(0, nullptr);
        else
            m_Renderer2DData.m_DescriptorSet[m_Renderer2DData.m_BatchDrawCallIndex][1]->SetTexture(0, m_Renderer2DData.m_Textures[0]);

        m_Renderer2DData.m_DescriptorSet[m_Renderer2DData.m_BatchDrawCallIndex][1]->Update();

        m_Renderer2DData.m_PreviousFrameTextureCount[m_Renderer2DData.m_BatchDrawCallIndex] = m_Renderer2DData.m_TextureCount;

        Arena* frameArena                  = Application::Get().GetFrameArena();
        DescriptorSet** currentDescriptors = PushArrayNoZero(frameArena, DescriptorSet*, 2);

        currentDescriptors[0] = m_Renderer2DData.m_DescriptorSet[m_Renderer2DData.m_BatchDrawCallIndex][0].get();
        currentDescriptors[1] = m_Renderer2DData.m_DescriptorSet[m_Renderer2DData.m_BatchDrawCallIndex][1].get();

        m_Renderer2DData.m_IndexBuffer->SetCount(m_Renderer2DData.m_IndexCount);
        m_Renderer2DData.m_IndexBuffer->Bind(commandBuffer);

        Renderer::BindDescriptorSets(m_Renderer2DData.m_Pipeline.get(), commandBuffer, 0, currentDescriptors, 2);
        Renderer::DrawIndexed(commandBuffer, DrawType::TRIANGLE, m_Renderer2DData.m_IndexCount);

        m_Renderer2DData.m_VertexBuffers[currentFrame][m_Renderer2DData.m_BatchDrawCallIndex]->Unbind();
        m_Renderer2DData.m_IndexBuffer->Unbind();

        m_Renderer2DData.m_BatchDrawCallIndex++;
        m_Renderer2DData.m_TextureCount = 0;
    }

    void SceneRenderer::Init2DLitRenderData()
    {
        m_LitData.m_IndexCount           = 0;
        m_LitData.m_Buffer               = nullptr;
        m_LitData.m_RenderToDepthTexture = true;
        m_LitData.m_TriangleIndicies     = false;
        m_LitData.m_Limits.SetMaxQuads(10000);
        m_LitData.m_Limits.MaxTextures = 16;

        m_LitData.m_Shader = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("Batch2DLit")).As<Graphics::Shader>();
        if(!m_LitData.m_Shader)
        {
            LWARN("Batch2DLit shader not found - 2D lighting disabled");
            return;
        }

        m_LitData.m_TransformationStack.EmplaceBack(Mat4(1.0f));
        m_LitData.m_TransformationBack = &m_LitData.m_TransformationStack.Back();

        Graphics::DescriptorDesc descriptorDesc {};
        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader      = m_LitData.m_Shader.get();
        m_LitData.m_DescriptorSet.Resize(m_LitData.m_Limits.MaxBatchDrawCalls);
        m_LitData.m_PreviousFrameTextureCount.Resize(m_LitData.m_Limits.MaxBatchDrawCalls);

        for(uint32_t i = 0; i < m_LitData.m_Limits.MaxBatchDrawCalls; i++)
        {
            m_LitData.m_PreviousFrameTextureCount[i] = 0;
            m_LitData.m_DescriptorSet[i].Resize(2);
            m_LitData.m_DescriptorSet[i][0] = nullptr;
            m_LitData.m_DescriptorSet[i][1] = nullptr;
        }
        m_LitData.m_DescriptorSet[0][0] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));

        m_LitData.m_VertexBuffers.Resize(Renderer::GetMainSwapChain()->GetSwapChainBufferCount());

        uint32_t* indices = new uint32_t[m_LitData.m_Limits.IndiciesSize];
        uint32_t offset   = 0;
        for(uint32_t i = 0; i < m_LitData.m_Limits.IndiciesSize; i += 6)
        {
            indices[i]     = offset + 0;
            indices[i + 1] = offset + 1;
            indices[i + 2] = offset + 2;
            indices[i + 3] = offset + 2;
            indices[i + 4] = offset + 3;
            indices[i + 5] = offset + 0;
            offset += 4;
        }
        m_LitData.m_IndexBuffer = IndexBuffer::Create(indices, m_LitData.m_Limits.IndiciesSize);
        m_LitBufferBase.Resize(Renderer::GetMainSwapChain()->GetSwapChainBufferCount());

        for(int currentFrame = 0; currentFrame < Renderer::GetMainSwapChain()->GetSwapChainBufferCount(); currentFrame++)
        {
            m_LitData.m_VertexBuffers[currentFrame].EmplaceBack(Graphics::VertexBuffer::Create(BATCH_BUFFER_INITIAL, nullptr, BufferUsage::DYNAMIC));
            m_LitBufferBase[currentFrame].EmplaceBack(new VertexData[m_LitData.m_Limits.MaxQuads * 4]);
        }

        delete[] indices;
        m_LitDataInitialised = true;
    }

    float SceneRenderer::SubmitLitTexture(Texture* texture)
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        for(uint32_t i = 0; i < m_LitData.m_TextureCount; i++)
        {
            if(m_LitData.m_Textures[i] == texture)
                return static_cast<float>(i + 1);
        }

        if(m_LitData.m_TextureCount >= m_LitData.m_Limits.MaxTextures)
        {
            Lit2DFlush();
            Lit2DBeginBatch();
        }
        m_LitData.m_Textures[m_LitData.m_TextureCount] = texture;
        m_LitData.m_TextureCount++;
        return static_cast<float>(m_LitData.m_TextureCount);
    }

    void SceneRenderer::Lit2DBeginBatch()
    {
        uint32_t currentFrame = Renderer::GetMainSwapChain()->GetCurrentBufferIndex();

        m_LitData.m_IndexCount   = 0;
        m_LitData.m_TextureCount = 0;

        if((int)m_LitData.m_VertexBuffers[currentFrame].Size() - 1 < (int)m_LitData.m_BatchDrawCallIndex)
        {
            auto& vertexBuffer = m_LitData.m_VertexBuffers[currentFrame].EmplaceBack(Graphics::VertexBuffer::Create(BufferUsage::DYNAMIC));
            vertexBuffer->Resize(BATCH_BUFFER_INITIAL);
        }

        m_LitData.m_Buffer = m_LitBufferBase[currentFrame][0];

        Mat4 projView = m_Camera->GetGPUProjectionMatrix() * m_CameraTransform->GetWorldMatrix().Inverse();

        if(m_LitData.m_DescriptorSet[m_LitData.m_BatchDrawCallIndex][0] == nullptr)
        {
            Graphics::DescriptorDesc descriptorDesc {};
            descriptorDesc.layoutIndex                                = 0;
            descriptorDesc.shader                                     = m_LitData.m_Shader.get();
            m_LitData.m_DescriptorSet[m_LitData.m_BatchDrawCallIndex][0] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
        }

        auto& set = m_LitData.m_DescriptorSet[m_LitData.m_BatchDrawCallIndex][0];
        set->SetUniformBufferData(0, &projView);
        set->SetUniformBufferData(1, &m_Lights2D);
        set->Update();
    }

    void SceneRenderer::Render2DLitPass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("Render2D Lit Pass");

        if(!m_LitDataInitialised || !m_LitData.m_Shader)
            return;

        if(m_LitData.m_CommandQueue2D.Empty())
            return;

        Graphics::PipelineDesc pipelineDesc;
        pipelineDesc.shader              = m_LitData.m_Shader;
        pipelineDesc.polygonMode         = Graphics::PolygonMode::FILL;
        pipelineDesc.cullMode            = Graphics::CullMode::NONE;
        pipelineDesc.transparencyEnabled = true;
        pipelineDesc.blendMode           = BlendMode::SrcAlphaOneMinusSrcAlpha;
        pipelineDesc.clearTargets        = false;
        pipelineDesc.depthTarget         = reinterpret_cast<Texture*>(m_ForwardData.m_DepthTexture);
        pipelineDesc.colourTargets[0]    = m_MainTexture;
        pipelineDesc.DepthWrite          = false;
        pipelineDesc.DebugName           = "2D Lit";
        pipelineDesc.samples             = m_MainTextureSamples;
        if(m_MainTextureSamples > 1)
            pipelineDesc.resolveTexture = m_ResolveTexture;

        m_LitData.m_Pipeline = Graphics::Pipeline::Get(pipelineDesc);

        Lit2DBeginBatch();

        for(auto& command : m_LitData.m_CommandQueue2D)
        {
            m_Stats.NumRenderedObjects++;

            if(m_LitData.m_IndexCount >= m_LitData.m_Limits.IndiciesSize)
            {
                Lit2DFlush();
                Lit2DBeginBatch();
            }

            auto& renderable = command.renderable;
            auto& transform  = command.transform;

            const Vec2 min    = renderable->GetPosition();
            const Vec2 max    = renderable->GetPosition() + renderable->GetScale();
            const Vec4 colour = renderable->GetColour();
            const auto& uv    = renderable->GetUVs();
            Texture* texture  = renderable->GetTexture();
            Texture* normal   = renderable->GetNormalTexture();

            float albedoSlot = 0.0f;
            if(texture)
                albedoSlot = SubmitLitTexture(texture);
            float normalSlot = 0.0f;
            if(normal)
                normalSlot = SubmitLitTexture(normal);

            const Vec2 tid = Vec2(albedoSlot, normalSlot);

            Vec3 vertex                = transform * Vec4(min.x, min.y, 0.0f, 1.0f);
            m_LitData.m_Buffer->vertex = vertex;
            m_LitData.m_Buffer->uv     = Vec4(uv[0].x, uv[0].y, 0.0f, 0.0f);
            m_LitData.m_Buffer->tid    = tid;
            m_LitData.m_Buffer->colour = colour;
            m_LitData.m_Buffer++;

            vertex                     = transform * Vec4(max.x, min.y, 0.0f, 1.0f);
            m_LitData.m_Buffer->vertex = vertex;
            m_LitData.m_Buffer->uv     = Vec4(uv[1].x, uv[1].y, 0.0f, 0.0f);
            m_LitData.m_Buffer->tid    = tid;
            m_LitData.m_Buffer->colour = colour;
            m_LitData.m_Buffer++;

            vertex                     = transform * Vec4(max.x, max.y, 0.0f, 1.0f);
            m_LitData.m_Buffer->vertex = vertex;
            m_LitData.m_Buffer->uv     = Vec4(uv[2].x, uv[2].y, 0.0f, 0.0f);
            m_LitData.m_Buffer->tid    = tid;
            m_LitData.m_Buffer->colour = colour;
            m_LitData.m_Buffer++;

            vertex                     = transform * Vec4(min.x, max.y, 0.0f, 1.0f);
            m_LitData.m_Buffer->vertex = vertex;
            m_LitData.m_Buffer->uv     = Vec4(uv[3].x, uv[3].y, 0.0f, 0.0f);
            m_LitData.m_Buffer->tid    = tid;
            m_LitData.m_Buffer->colour = colour;
            m_LitData.m_Buffer++;

            m_LitData.m_IndexCount += 6;
        }

        if(m_LitData.m_IndexCount == 0)
            return;

        Lit2DFlush();
    }

    void SceneRenderer::Lit2DFlush()
    {
        LUMOS_PROFILE_FUNCTION();
        uint32_t currentFrame                  = Renderer::GetMainSwapChain()->GetCurrentBufferIndex();
        Graphics::CommandBuffer* commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
        // keep the render pass open between batches: ending it reloads / stores the whole target

        uint32_t dataSize = (uint32_t)((uint8_t*)m_LitData.m_Buffer - (uint8_t*)m_LitBufferBase[currentFrame][0]);
        UploadBatch(m_LitData.m_VertexBuffers[currentFrame][m_LitData.m_BatchDrawCallIndex], dataSize, m_LitBufferBase[currentFrame][0]);
        commandBuffer->BindPipeline(m_LitData.m_Pipeline);
        m_LitData.m_VertexBuffers[currentFrame][m_LitData.m_BatchDrawCallIndex]->Bind(commandBuffer, m_LitData.m_Pipeline.get());

        if(m_LitData.m_DescriptorSet[m_LitData.m_BatchDrawCallIndex][1] == nullptr)
        {
            Graphics::DescriptorDesc descriptorDesc {};
            descriptorDesc.layoutIndex                                = 1;
            descriptorDesc.shader                                     = m_LitData.m_Shader.get();
            m_LitData.m_DescriptorSet[m_LitData.m_BatchDrawCallIndex][1] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
        }

        for(uint32_t i = m_LitData.m_TextureCount; i < 16; i++)
            m_LitData.m_Textures[i] = Material::GetDefaultTexture();
        m_LitData.m_TextureCount = 16;

        m_LitData.m_DescriptorSet[m_LitData.m_BatchDrawCallIndex][1]->SetTexture(0, m_LitData.m_Textures, m_LitData.m_TextureCount);
        m_LitData.m_DescriptorSet[m_LitData.m_BatchDrawCallIndex][1]->Update();

        Arena* frameArena                  = Application::Get().GetFrameArena();
        DescriptorSet** currentDescriptors = PushArrayNoZero(frameArena, DescriptorSet*, 2);
        currentDescriptors[0]              = m_LitData.m_DescriptorSet[m_LitData.m_BatchDrawCallIndex][0].get();
        currentDescriptors[1]              = m_LitData.m_DescriptorSet[m_LitData.m_BatchDrawCallIndex][1].get();

        m_LitData.m_IndexBuffer->SetCount(m_LitData.m_IndexCount);
        m_LitData.m_IndexBuffer->Bind(commandBuffer);

        Renderer::BindDescriptorSets(m_LitData.m_Pipeline.get(), commandBuffer, 0, currentDescriptors, 2);
        Renderer::DrawIndexed(commandBuffer, DrawType::TRIANGLE, m_LitData.m_IndexCount);

        m_LitData.m_VertexBuffers[currentFrame][m_LitData.m_BatchDrawCallIndex]->Unbind();
        m_LitData.m_IndexBuffer->Unbind();

        m_LitData.m_BatchDrawCallIndex++;
        m_LitData.m_TextureCount = 0;
    }

    void SceneRenderer::TextFlush(Renderer2DData& textRenderData, TDArray<TextVertexData*>& textVertexBufferBase, TextVertexData*& textVertexBufferPtr)
    {
        LUMOS_PROFILE_FUNCTION();
        if(textRenderData.m_BatchDrawCallIndex >= textRenderData.m_Limits.MaxBatchDrawCalls)
        {
            static u64 s_LastWarnFrame = ~0ull;
            if(s_LastWarnFrame != Engine::Get().Statistics().FramesPerSecond)
            {
                s_LastWarnFrame = Engine::Get().Statistics().FramesPerSecond;
                LWARN("Text batch draw call limit (%u) hit - dropping draws", textRenderData.m_Limits.MaxBatchDrawCalls);
            }
            // rewind the write pointer too: left alone, the next glyphs ran off the end of the vertex buffer
            textVertexBufferPtr           = textVertexBufferBase[Renderer::GetMainSwapChain()->GetCurrentBufferIndex()];
            textRenderData.m_IndexCount   = 0;
            textRenderData.m_TextureCount = 0;
            return;
        }

        uint32_t currentFrame = Renderer::GetMainSwapChain()->GetCurrentBufferIndex();

        if(textRenderData.m_DescriptorSet[textRenderData.m_BatchDrawCallIndex][1] == nullptr)
        {
            /*
             || m_Renderer2DData.m_TextureCount != m_Renderer2DData.m_PreviousFrameTextureCount[m_Renderer2DData.m_BatchDrawCallIndex])
             When previous frame texture count was less then than the previous frame
             and the texture previously used was deleted, there was a crash - maybe moltenvk only
             May not be needed anymore
            */
            Graphics::DescriptorDesc descriptorDesc {};
            descriptorDesc.layoutIndex                                             = 1;
            descriptorDesc.shader                                                  = textRenderData.m_Shader.get();
            textRenderData.m_DescriptorSet[textRenderData.m_BatchDrawCallIndex][1] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
        }

        for(int i = textRenderData.m_TextureCount; i < 16; i++)
        {
            textRenderData.m_Textures[i] = Material::GetDefaultTexture();
        }

        textRenderData.m_TextureCount = 16;

        if(textRenderData.m_TextureCount > 1)
            textRenderData.m_DescriptorSet[textRenderData.m_BatchDrawCallIndex][1]->SetTexture(0, textRenderData.m_Textures, textRenderData.m_TextureCount);
        else if(textRenderData.m_TextureCount == 0)
            textRenderData.m_DescriptorSet[textRenderData.m_BatchDrawCallIndex][1]->SetTexture(0, Material::GetDefaultTexture());
        else
            textRenderData.m_DescriptorSet[textRenderData.m_BatchDrawCallIndex][1]->SetTexture(0, textRenderData.m_Textures[0]);

        textRenderData.m_DescriptorSet[textRenderData.m_BatchDrawCallIndex][1]->Update();

        textRenderData.m_PreviousFrameTextureCount[textRenderData.m_BatchDrawCallIndex] = textRenderData.m_TextureCount;

        Graphics::CommandBuffer* commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();

        if((int)textRenderData.m_VertexBuffers[currentFrame].Size() - 1 < (int)textRenderData.m_BatchDrawCallIndex)
        {
            auto& vertexBuffer = textRenderData.m_VertexBuffers[currentFrame].EmplaceBack(Graphics::VertexBuffer::Create(BATCH_BUFFER_INITIAL, nullptr, BufferUsage::DYNAMIC));
        }

        // keep the render pass open between batches: ending it reloads / stores the whole target

        uint32_t dataSize = (uint32_t)((uint8_t*)textVertexBufferPtr - (uint8_t*)textVertexBufferBase[currentFrame]);
        UploadBatch(textRenderData.m_VertexBuffers[currentFrame][textRenderData.m_BatchDrawCallIndex], dataSize, textVertexBufferBase[currentFrame]);

        commandBuffer->BindPipeline(textRenderData.m_Pipeline);

        Arena* frameArena                  = Application::Get().GetFrameArena();
        DescriptorSet** currentDescriptors = PushArrayNoZero(frameArena, DescriptorSet*, 2);

        currentDescriptors[0] = textRenderData.m_DescriptorSet[m_TextRendererData.m_BatchDrawCallIndex][0].get();
        currentDescriptors[1] = textRenderData.m_DescriptorSet[textRenderData.m_BatchDrawCallIndex][1].get();

        textRenderData.m_VertexBuffers[currentFrame][textRenderData.m_BatchDrawCallIndex]->Bind(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), textRenderData.m_Pipeline.get());

        textRenderData.m_IndexBuffer->SetCount(textRenderData.m_IndexCount);
        textRenderData.m_IndexBuffer->Bind(commandBuffer);

        Renderer::BindDescriptorSets(textRenderData.m_Pipeline.get(), commandBuffer, 0, currentDescriptors, 2);
        Renderer::DrawIndexed(commandBuffer, DrawType::TRIANGLE, textRenderData.m_IndexCount);

        textRenderData.m_VertexBuffers[currentFrame][textRenderData.m_BatchDrawCallIndex]->Unbind();
        textRenderData.m_IndexBuffer->Unbind();

        textVertexBufferPtr         = textVertexBufferBase[currentFrame];
        textRenderData.m_IndexCount = 0;
        textRenderData.m_BatchDrawCallIndex++;

        textRenderData.m_TextureCount = 0;
    }

    void SceneRenderer::TextPass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("Text Pass");

        if(!m_Camera)
            return;

        auto textGroup = m_CurrentScene->GetRegistry().group<TextComponent>(entt::get<Maths::Transform>);
        if(textGroup.size() == 0)
            return;

        Graphics::PipelineDesc pipelineDesc;
        pipelineDesc.shader              = m_TextRendererData.m_Shader;
        pipelineDesc.polygonMode         = Graphics::PolygonMode::FILL;
        pipelineDesc.cullMode            = Graphics::CullMode::BACK;
        pipelineDesc.transparencyEnabled = true;
        pipelineDesc.blendMode           = BlendMode::SrcAlphaOneMinusSrcAlpha;
        pipelineDesc.clearTargets        = false;
        pipelineDesc.colourTargets[0]    = m_LastRenderTarget;
        pipelineDesc.DebugName           = "Text";
        pipelineDesc.samples             = 1;

        // if(m_MainTextureSamples > 1)
        //     pipelineDesc.resolveTexture = m_ResolveTexture;

        m_TextRendererData.m_Pipeline = Graphics::Pipeline::Get(pipelineDesc);

        uint32_t currentFrame = Renderer::GetMainSwapChain()->GetCurrentBufferIndex();

        if((int)m_TextRendererData.m_VertexBuffers[currentFrame].Size() - 1 < (int)m_TextRendererData.m_BatchDrawCallIndex)
        {
            auto& vertexBuffer = m_TextRendererData.m_VertexBuffers[currentFrame].EmplaceBack(Graphics::VertexBuffer::Create(BATCH_BUFFER_INITIAL, nullptr, BufferUsage::DYNAMIC));
        }

        m_TextRendererData.m_VertexBuffers[currentFrame][m_TextRendererData.m_BatchDrawCallIndex]->Bind(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), m_TextRendererData.m_Pipeline.get());
        TextVertexBufferPtr = TextVertexBufferBase[currentFrame];
        // m_TextBuffer = m_TextRendererData.m_VertexBuffers[currentFrame][m_TextRendererData.m_BatchDrawCallIndex]->GetPointer<TextVertexData>();

        auto projView = m_Camera->GetGPUProjectionMatrix() * m_CameraTransform->GetWorldMatrix().Inverse();
        m_TextRendererData.m_DescriptorSet[m_TextRendererData.m_BatchDrawCallIndex][0]->SetUniformBufferData(0, &projView);
        m_TextRendererData.m_DescriptorSet[m_TextRendererData.m_BatchDrawCallIndex][0]->Update();

        m_TextRendererData.m_TextureCount = 0;
        for(auto entity : textGroup)
        {
            const auto& [textComp, trans] = textGroup.get<TextComponent, Maths::Transform>(entity);

            auto font = textComp.FontHandle ? textComp.FontHandle : Font::GetDefaultFont();
            if(!font || !font->GetFontAtlas())
                continue;

            Vec2 textSize = font->CalculateTextSize(textComp.TextString, 1.0f);

            Maths::BoundingBox textBB;
            textBB.Merge(Vec3(0.0f, -textSize.y, 0.0f));
            textBB.Merge(Vec3(textSize.x, textSize.y, 0.0f));
            textBB.Transform(trans.GetWorldMatrix());

            if(!m_ForwardData.m_Frustum.IsInside(textBB))
                continue;

            Mat4 transform = trans.GetWorldMatrix();
            m_Stats.NumRenderedObjects++;

            if(m_TextRendererData.m_IndexCount >= m_TextRendererData.m_Limits.IndiciesSize)
                TextFlush(m_TextRendererData, TextVertexBufferBase, TextVertexBufferPtr);

            int textureIndex       = -1;
            auto& string           = textComp.TextString;
            float lineHeightOffset = 0.0f;
            float kerningOffset    = 0.0f;

            float maxWidth     = textComp.MaxWidth;
            auto colour        = textComp.Colour;
            float lineSpacing  = textComp.LineSpacing;
            float kerning      = textComp.Kerning;
            auto outlineColour = textComp.OutlineColour;
            auto outlineWidth  = textComp.OutlineWidth;

            SharedPtr<Texture2D> fontAtlas = font->GetFontAtlas();

            for(uint32_t i = 0; i < m_TextRendererData.m_TextureCount; i++)
            {
                if(m_TextRendererData.m_Textures[i] == fontAtlas.get())
                {
                    textureIndex = int(i + 1);
                    break;
                }
            }

            if(textureIndex == -1)
            {
                textureIndex                                                     = (int)m_TextRendererData.m_TextureCount + 1;
                m_TextRendererData.m_Textures[m_TextRendererData.m_TextureCount] = fontAtlas.get();
                m_TextRendererData.m_TextureCount++;
            }

            auto& fontGeometry  = font->GetMSDFData()->FontGeometry;
            const auto& metrics = fontGeometry.getMetrics();

            {
                double x       = 0.0;
                double fsScale = 1 / (metrics.ascenderY - metrics.descenderY);
                double y       = 0.0;
                const MSDFData* msdfData = font->GetMSDFData();
                for(int i = 0; i < string.size(); i++)
                {
                    UnicodeDecode dec  = Utf8Decode((u8*)string.data() + i, string.size() - i);
                    char32_t character = dec.codepoint;
                    if(dec.inc > 1)
                        i += dec.inc - 1;

                    if(character == '\r')
                        continue;

                    if(character == '\n')
                    {
                        x = 0;
                        y -= fsScale * metrics.lineHeight + lineHeightOffset;
                        continue;
                    }

                    if(character == '\t')
                    {
                        auto glyph     = fontGeometry.getGlyph('a');
                        double advance = glyph->getAdvance();
                        x += 4 * fsScale * advance + kerningOffset;
                        continue;
                    }

                    auto glyph = msdfData->GetGlyph(character);
                    if(!glyph)
                        glyph = fontGeometry.getGlyph('?');
                    if(!glyph)
                        continue;

                    double l, b, r, t;
                    glyph->getQuadAtlasBounds(l, b, r, t);

                    double pl, pb, pr, pt;
                    glyph->getQuadPlaneBounds(pl, pb, pr, pt);

                    pl *= fsScale, pb *= fsScale, pr *= fsScale, pt *= fsScale;
                    pl += x, pb += y, pr += x, pt += y;

                    double texelWidth  = 1. / fontAtlas->GetWidth();
                    double texelHeight = 1. / fontAtlas->GetHeight();
                    l *= texelWidth, b *= texelHeight, r *= texelWidth, t *= texelHeight;

                    {
                        LUMOS_PROFILE_SCOPE("Set text buffer data");
                        TextVertexBufferPtr->vertex        = transform * Vec4((float)pl, (float)pb, 0.0f, 1.0f);
                        TextVertexBufferPtr->colour        = colour;
                        TextVertexBufferPtr->uv            = { (float)l, (float)b };
                        TextVertexBufferPtr->tid           = Vec2((float)textureIndex, outlineWidth);
                        TextVertexBufferPtr->outlineColour = outlineColour;
                        TextVertexBufferPtr++;

                        TextVertexBufferPtr->vertex        = transform * Vec4((float)pr, (float)pb, 0.0f, 1.0f);
                        TextVertexBufferPtr->colour        = colour;
                        TextVertexBufferPtr->uv            = { (float)r, (float)b };
                        TextVertexBufferPtr->tid           = Vec2((float)textureIndex, outlineWidth);
                        TextVertexBufferPtr->outlineColour = outlineColour;
                        TextVertexBufferPtr++;

                        TextVertexBufferPtr->vertex        = transform * Vec4((float)pr, (float)pt, 0.0f, 1.0f);
                        TextVertexBufferPtr->colour        = colour;
                        TextVertexBufferPtr->uv            = { (float)r, (float)t };
                        TextVertexBufferPtr->tid           = Vec2((float)textureIndex, outlineWidth);
                        TextVertexBufferPtr->outlineColour = outlineColour;
                        TextVertexBufferPtr++;

                        TextVertexBufferPtr->vertex        = transform * Vec4((float)pl, (float)pt, 0.0f, 1.0f);
                        TextVertexBufferPtr->colour        = colour;
                        TextVertexBufferPtr->uv            = { (float)l, (float)t };
                        TextVertexBufferPtr->tid           = Vec2((float)textureIndex, outlineWidth);
                        TextVertexBufferPtr->outlineColour = outlineColour;
                        TextVertexBufferPtr++;
                    }

                    m_TextRendererData.m_IndexCount += 6;

                    double advance = glyph->getAdvance();
                    // Next *codepoint* for kerning, not the next raw byte.
                    char32_t next = (i + 1 < (int)string.size()) ? Utf8Decode((u8*)string.data() + i + 1, string.size() - i - 1).codepoint : 0;
                    fontGeometry.getAdvance(advance, character, next);
                    x += fsScale * advance + kerningOffset;
                }
            }
        }

        if(m_TextRendererData.m_IndexCount == 0)
        {
            // m_TextRendererData.m_VertexBuffers[currentFrame][m_TextRendererData.m_BatchDrawCallIndex]->ReleasePointer();
            return;
        }

        TextFlush(m_TextRendererData, TextVertexBufferBase, TextVertexBufferPtr);

        // m_TextRendererData.m_VertexBuffers[currentFrame][m_TextRendererData.m_BatchDrawCallIndex]->ReleasePointer();
    }

    void SceneRenderer::WorldTextPass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("World Text Pass");

        if(!m_Camera || !m_CameraTransform)
        {
            Graphics::ClearWorldLabels();
            return;
        }

        auto& labels = Graphics::GetActiveWorldLabels();
        if(labels.Empty())
            return;

        auto font = Font::GetDefaultFont();
        if(!font || !font->GetFontAtlas())
        {
            Graphics::ClearWorldLabels();
            return;
        }

        Graphics::PipelineDesc pipelineDesc;
        pipelineDesc.shader              = m_TextRendererData.m_Shader;
        pipelineDesc.polygonMode         = Graphics::PolygonMode::FILL;
        pipelineDesc.cullMode            = Graphics::CullMode::BACK;
        pipelineDesc.transparencyEnabled = true;
        pipelineDesc.blendMode           = BlendMode::SrcAlphaOneMinusSrcAlpha;
        pipelineDesc.clearTargets        = false;
        pipelineDesc.colourTargets[0]    = m_LastRenderTarget;
        pipelineDesc.DebugName           = "World Text";
        pipelineDesc.samples             = 1;

        m_TextRendererData.m_Pipeline = Graphics::Pipeline::Get(pipelineDesc);

        uint32_t currentFrame = Renderer::GetMainSwapChain()->GetCurrentBufferIndex();

        // World frame + optional local "system frame" (to-scale labels).
        Mat4 worldProjView = m_Camera->GetGPUProjectionMatrix() * m_CameraTransform->GetWorldMatrix().Inverse();
        bool haveLocal     = Graphics::SystemFrameEnabled();
        Mat4 localProjView = worldProjView;
        if(haveLocal)
        {
            Mat4 localWorld = m_CameraTransform->GetWorldMatrix();
            localWorld.SetTranslation(Graphics::SystemFrameCamLocal());
            localProjView = ReversedPerspective(1e-4f, 1e5f, m_Camera->GetAspectRatio(), m_Camera->GetFOV()) * localWorld.Inverse();
        }

        // Camera basis for the billboard (same world-space directions in both frames).
        Vec3 right = m_CameraTransform->GetRightDirection();
        Vec3 up    = m_CameraTransform->GetUpDirection();

        const MSDFData* msdfData       = font->GetMSDFData();
        auto& fontGeometry             = msdfData->FontGeometry;
        const auto& metrics            = fontGeometry.getMetrics();
        SharedPtr<Texture2D> fontAtlas = font->GetFontAtlas();
        double fsScale     = 1.0 / (metrics.ascenderY - metrics.descenderY);
        double texelWidth  = 1.0 / fontAtlas->GetWidth();
        double texelHeight = 1.0 / fontAtlas->GetHeight();

        // Render all labels of one frame (local or world) as a batch.
        auto emitBatch = [&](bool localPass, const Mat4& projView)
        {
            // Each batch slot has its own vertex buffer and descriptor set, so this runs
            // again after a flush part-way through the labels moves on to the next slot.
            int textureIndex = 1;
            auto beginBatch  = [&]() -> bool
            {
                if(m_TextRendererData.m_BatchDrawCallIndex >= m_TextRendererData.m_Limits.MaxBatchDrawCalls)
                    return false;

                if((int)m_TextRendererData.m_VertexBuffers[currentFrame].Size() - 1 < (int)m_TextRendererData.m_BatchDrawCallIndex)
                    m_TextRendererData.m_VertexBuffers[currentFrame].EmplaceBack(Graphics::VertexBuffer::Create(BATCH_BUFFER_INITIAL, nullptr, BufferUsage::DYNAMIC));

                m_TextRendererData.m_VertexBuffers[currentFrame][m_TextRendererData.m_BatchDrawCallIndex]->Bind(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), m_TextRendererData.m_Pipeline.get());
                TextVertexBufferPtr = TextVertexBufferBase[currentFrame];

                m_TextRendererData.m_DescriptorSet[m_TextRendererData.m_BatchDrawCallIndex][0]->SetUniformBufferData(0, (void*)&projView);
                m_TextRendererData.m_DescriptorSet[m_TextRendererData.m_BatchDrawCallIndex][0]->Update();

                m_TextRendererData.m_IndexCount   = 0;
                m_TextRendererData.m_TextureCount = 0;
                m_TextRendererData.m_Textures[m_TextRendererData.m_TextureCount] = fontAtlas.get();
                m_TextRendererData.m_TextureCount++;
                return true;
            };

            if(!beginBatch())
                return;

            // Back to front, or a far label drawn later covers a near one.
            struct LabelOrder
            {
                float depth;
                u32 index;
            };
            ArenaTemp scratch = ScratchBegin(nullptr, 0);
            LabelOrder* order = PushArrayNoZero(scratch.arena, LabelOrder, labels.Size());
            u32 orderCount    = 0;
            const Vec3 eye    = localPass ? Graphics::SystemFrameCamLocal() : m_CameraTransform->GetWorldPosition();
            const Vec3 fwd    = Maths::Cross(up, right);
            for(u32 i = 0; i < (u32)labels.Size(); i++)
            {
                if(labels[i].Local != localPass)
                    continue;
                order[orderCount++] = { Maths::Dot(labels[i].Position - eye, fwd), i };
            }
            // index breaks ties so labels at one position keep the order they were added in
            Algorithms::IntroSort(order, order + orderCount, [](const LabelOrder& a, const LabelOrder& b)
                                  { return a.depth != b.depth ? a.depth > b.depth : a.index < b.index; });

            const bool any          = orderCount > 0;
            const double lineHeight = fsScale * metrics.lineHeight;

            auto measureLine = [&](const char* text, u64 len) -> double
            {
                double width = 0.0;
                for(u64 i = 0; i < len; i++)
                {
                    UnicodeDecode dec = Utf8Decode((u8*)text + i, len - i);
                    if(dec.inc > 1) i += dec.inc - 1;
                    auto glyph = msdfData->GetGlyph(dec.codepoint);
                    if(!glyph) glyph = fontGeometry.getGlyph('?');
                    if(!glyph) continue;
                    double advance = glyph->getAdvance();
                    char32_t next  = (i + 1 < len) ? Utf8Decode((u8*)text + i + 1, len - i - 1).codepoint : 0;
                    fontGeometry.getAdvance(advance, dec.codepoint, next);
                    width += fsScale * advance;
                }
                return width;
            };

            for(u32 o = 0; o < orderCount; o++)
            {
                const WorldLabel& label = labels[order[o].index];

                const u64 textLen = (u64)strlen(label.Text);
                const float s     = label.Size;
                const Vec4 colour = label.Colour;
                const Vec3 origin = label.Position;

                // Lines are centred on the label position; the first line sits on it and the rest hang below.
                double blockWidth = 0.0;
                u32 lineCount     = 1;
                for(u64 start = 0, i = 0; i <= textLen; i++)
                {
                    if(i == textLen || label.Text[i] == '\n')
                    {
                        blockWidth = Maths::Max(blockWidth, measureLine(label.Text + start, i - start));
                        if(i < textLen)
                            lineCount++;
                        start = i + 1;
                    }
                }

                // worst case one quad per byte plus the backing
                if(m_TextRendererData.m_IndexCount + (textLen + 1) * 6 >= m_TextRendererData.m_Limits.IndiciesSize)
                {
                    TextFlush(m_TextRendererData, TextVertexBufferBase, TextVertexBufferPtr);
                    if(!beginBatch())
                        break; // out of batch slots this frame: drop the rest
                }

                if(label.Background.w > 0.0f)
                {
                    // one backing quad for the whole block
                    const float padX = 0.30f, padY = 0.10f; // em units
                    const float qx0  = (float)(-blockWidth * 0.5) - padX;
                    const float qx1  = (float)(blockWidth * 0.5) + padX;
                    const float qy0  = (float)(fsScale * metrics.descenderY - lineHeight * (lineCount - 1)) - padY;
                    const float qy1  = (float)(fsScale * metrics.ascenderY) + padY;
                    auto placeBg     = [&](float gx, float gy)
                    {
                        Vec3 w = origin + right * (gx * s) + up * (gy * s);
                        TextVertexBufferPtr->vertex        = Vec4(w, 1.0f);
                        TextVertexBufferPtr->colour        = label.Background;
                        TextVertexBufferPtr->uv            = { 0.0f, 0.0f };
                        TextVertexBufferPtr->tid           = Vec2(-1.0f, 0.0f);
                        TextVertexBufferPtr->outlineColour = Vec4(0.0f);
                        TextVertexBufferPtr++;
                    };
                    placeBg(qx0, qy0);
                    placeBg(qx1, qy0);
                    placeBg(qx1, qy1);
                    placeBg(qx0, qy1);
                    m_TextRendererData.m_IndexCount += 6;
                }

                double y = 0.0;
                for(u64 start = 0, end = 0; end <= textLen; end++)
                {
                    if(end != textLen && label.Text[end] != '\n')
                        continue;

                    const char* line   = label.Text + start;
                    const u64 lineLen  = end - start;
                    const double baseX = -measureLine(line, lineLen) * 0.5;
                    double x           = 0.0;

                    for(u64 i = 0; i < lineLen; i++)
                    {
                        UnicodeDecode dec = Utf8Decode((u8*)line + i, lineLen - i);
                        if(dec.inc > 1) i += dec.inc - 1;
                        auto glyph = msdfData->GetGlyph(dec.codepoint);
                        if(!glyph) glyph = fontGeometry.getGlyph('?');
                        if(!glyph) continue;

                        double al, ab, ar, at;
                        glyph->getQuadAtlasBounds(al, ab, ar, at);
                        double pl, pb, pr, pt;
                        glyph->getQuadPlaneBounds(pl, pb, pr, pt);

                        pl = pl * fsScale + x + baseX;
                        pr = pr * fsScale + x + baseX;
                        pb = pb * fsScale + y;
                        pt = pt * fsScale + y;
                        al *= texelWidth, ab *= texelHeight, ar *= texelWidth, at *= texelHeight;

                        auto place = [&](double gx, double gy, double u, double v)
                        {
                            Vec3 w = origin + right * (float)(gx * s) + up * (float)(gy * s);
                            TextVertexBufferPtr->vertex        = Vec4(w, 1.0f);
                            TextVertexBufferPtr->colour        = colour;
                            TextVertexBufferPtr->uv            = { (float)u, (float)v };
                            TextVertexBufferPtr->tid           = Vec2((float)textureIndex, 0.0f);
                            TextVertexBufferPtr->outlineColour = Vec4(0.0f);
                            TextVertexBufferPtr++;
                        };
                        place(pl, pb, al, ab);
                        place(pr, pb, ar, ab);
                        place(pr, pt, ar, at);
                        place(pl, pt, al, at);
                        m_TextRendererData.m_IndexCount += 6;

                        double advance = glyph->getAdvance();
                        char32_t next  = (i + 1 < lineLen) ? Utf8Decode((u8*)line + i + 1, lineLen - i - 1).codepoint : 0;
                        fontGeometry.getAdvance(advance, dec.codepoint, next);
                        x += fsScale * advance;
                    }

                    y -= lineHeight;
                    start = end + 1;
                }
            }
            ScratchEnd(scratch);

            if(any && m_TextRendererData.m_IndexCount > 0)
                TextFlush(m_TextRendererData, TextVertexBufferBase, TextVertexBufferPtr);
        };

        emitBatch(false, worldProjView);
        if(haveLocal)
            emitBatch(true, localProjView);

        Graphics::ClearWorldLabels();
    }

    void SceneRenderer::Begin2DPass()
    {
        // no unbind: 2D and text share the target, and ending the pass here reloads it per switch

        Graphics::PipelineDesc pipelineDesc;
        pipelineDesc.shader              = m_Renderer2DData.m_Shader;
        pipelineDesc.polygonMode         = Graphics::PolygonMode::FILL;
        pipelineDesc.cullMode            = Graphics::CullMode::BACK;
        pipelineDesc.transparencyEnabled = true;
        pipelineDesc.blendMode           = BlendMode::SrcAlphaOneMinusSrcAlpha;
        pipelineDesc.clearTargets        = false;
        // pipelineDesc.depthTarget = reinterpret_cast<Texture*>(m_ForwardData.m_DepthTexture);
        if(m_UISwapchainTarget)
            pipelineDesc.swapchainTarget = true;
        else
            pipelineDesc.colourTargets[0] = m_LastRenderTarget;
        pipelineDesc.DebugName        = "2D";
        pipelineDesc.samples          = 1; // m_MainTextureSamples;
        pipelineDesc.DepthTest        = false;
        if(pipelineDesc.samples > 1)
            pipelineDesc.resolveTexture = m_ResolveTexture;

        m_Renderer2DData.m_Pipeline = Graphics::Pipeline::Get(pipelineDesc);

        Renderer2DBeginBatch();
    }

    void SceneRenderer::BeginTextPass()
    {
        // no unbind: 2D and text share the target, and ending the pass here reloads it per switch

        Graphics::PipelineDesc pipelineDesc;
        pipelineDesc.shader              = m_TextRendererData.m_Shader;
        pipelineDesc.polygonMode         = Graphics::PolygonMode::FILL;
        pipelineDesc.cullMode            = Graphics::CullMode::BACK;
        pipelineDesc.transparencyEnabled = true;
        pipelineDesc.blendMode           = BlendMode::OneMinusSrcAlpha;
        pipelineDesc.clearTargets        = false;
        if(m_UISwapchainTarget)
            pipelineDesc.swapchainTarget = true;
        else
            pipelineDesc.colourTargets[0] = m_LastRenderTarget;
        pipelineDesc.DebugName           = "Text";
        pipelineDesc.DepthTest           = false;
        pipelineDesc.samples             = 1; // m_MainTextureSamples;

        if(pipelineDesc.samples > 1)
            pipelineDesc.resolveTexture = m_ResolveTexture;

        m_TextRendererData.m_Pipeline = Graphics::Pipeline::Get(pipelineDesc);

        uint32_t currentFrame = Renderer::GetMainSwapChain()->GetCurrentBufferIndex();

        if((int)m_TextRendererData.m_VertexBuffers[currentFrame].Size() - 1 < (int)m_TextRendererData.m_BatchDrawCallIndex)
        {
            auto& vertexBuffer = m_TextRendererData.m_VertexBuffers[currentFrame].EmplaceBack(Graphics::VertexBuffer::Create(BATCH_BUFFER_INITIAL, nullptr, BufferUsage::DYNAMIC));
        }

        m_TextRendererData.m_VertexBuffers[currentFrame][m_TextRendererData.m_BatchDrawCallIndex]->Bind(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), m_TextRendererData.m_Pipeline.get());
        TextVertexBufferPtr = TextVertexBufferBase[currentFrame];
        // m_TextBuffer = m_TextRendererData.m_VertexBuffers[currentFrame][m_TextRendererData.m_BatchDrawCallIndex]->GetPointer<TextVertexData>();

        // if (m_Camera)
        {
            // During UIPass, project in the UI's logical layout space; otherwise the target's pixels.
            float projWidth  = m_UIProjectionSize.x > 0.0f ? m_UIProjectionSize.x : (float)m_MainTexture->GetWidth();
            float projHeight = m_UIProjectionSize.y > 0.0f ? m_UIProjectionSize.y : (float)m_MainTexture->GetHeight();
            auto projView    = ReverseZ(Mat4::Orthographic(0.0f, projWidth, 0.0f, projHeight, -10.0f, 10.0f));

            m_TextRendererData.m_DescriptorSet[m_TextRendererData.m_BatchDrawCallIndex][0]->SetUniformBufferData(0, &projView);
        }
        m_TextRendererData.m_DescriptorSet[m_TextRendererData.m_BatchDrawCallIndex][0]->Update();

        m_TextRendererData.m_TextureCount = 0;
    }

    void SceneRenderer::DebugPass()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("Debug Pass");

        if(!m_Camera || !m_CameraTransform || !DebugRenderer::GetInstance())
            return;

        if(!m_DebugRenderDataInitialised)
        {
            LWARN("Debug Render data not initialised");
            return;
        }
        m_DebugDrawData.m_LineBatchDrawCallIndex              = 0;
        m_DebugDrawData.m_Renderer2DData.m_BatchDrawCallIndex = 0;
        m_DebugDrawData.m_PointBatchDrawCallIndex             = 0;
        // Loop twice for depth test and no depth test
        for(int i = 0; i < 2; i++)
        {
            bool depthTest = i == 0;

            auto& lines      = DebugRenderer::GetInstance()->GetLines(depthTest);
            auto& thickLines = DebugRenderer::GetInstance()->GetThickLines(depthTest);
            auto& triangles  = DebugRenderer::GetInstance()->GetTriangles(depthTest);
            auto& points     = DebugRenderer::GetInstance()->GetPoints(depthTest);

            auto projView = m_Camera->GetGPUProjectionMatrix() * Mat4::Inverse(m_CameraTransform->GetWorldMatrix());

            if(!lines.Empty())
            {
                LUMOS_PROFILE_SCOPE("Debug Lines");
                m_DebugDrawData.m_LineDescriptorSet[0]->SetUniformBufferData(0, &projView);
                m_DebugDrawData.m_LineDescriptorSet[0]->Update();

                Graphics::CommandBuffer* commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();

                Graphics::PipelineDesc pipelineDesc;
                pipelineDesc.shader = m_DebugDrawData.m_LineShader;

                pipelineDesc.polygonMode         = Graphics::PolygonMode::FILL;
                pipelineDesc.cullMode            = Graphics::CullMode::BACK;
                pipelineDesc.transparencyEnabled = true;
                pipelineDesc.blendMode           = BlendMode::SrcAlphaOneMinusSrcAlpha;
                pipelineDesc.clearTargets        = false;
                pipelineDesc.drawType            = DrawType::LINES;
                pipelineDesc.colourTargets[0]    = m_LastRenderTarget;
                // Pull debug lines toward the camera so they don't z-fight (look faint) against coincident geometry.
                pipelineDesc.depthBiasEnabled        = true;
                pipelineDesc.depthBiasConstantFactor = -1.25f;
                pipelineDesc.depthBiasSlopeFactor    = -1.75f;
                pipelineDesc.DebugName           = "Debug-Lines";
                pipelineDesc.samples = 1; // overlay: drawn after post-process onto the single-sample LDR target
                if(depthTest && m_MainTextureSamples <= 1)
                    pipelineDesc.depthTarget = m_ForwardData.m_DepthTexture;

                auto pipeline = Graphics::Pipeline::Get(pipelineDesc);
                Renderer::GetMainSwapChain()->GetCurrentCommandBuffer()->BindPipeline(pipeline);

                if(m_DebugDrawData.m_LineVertexBuffers.Empty())
                    m_DebugDrawData.m_LineVertexBuffers.EmplaceBack();
                if((int)m_DebugDrawData.m_LineVertexBuffers[0].Size() - 1 < (int)m_DebugDrawData.m_LineBatchDrawCallIndex)
                {
                    auto& vertexBuffer = m_DebugDrawData.m_LineVertexBuffers[0].EmplaceBack(Graphics::VertexBuffer::Create(RENDERER_LINE_BUFFER_SIZE, nullptr, BufferUsage::DYNAMIC));
                    // vertexBuffer->Resize(RENDERER_LINE_BUFFER_SIZE);
                }

                m_DebugDrawData.m_LineVertexBuffers[0][m_DebugDrawData.m_LineBatchDrawCallIndex]->Bind(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), pipeline.get());
                m_DebugDrawData.m_LineBuffer = m_LineBufferBase[0][0]; //[m_DebugDrawData.m_LineBatchDrawCallIndex];

                // m_DebugDrawData.m_LineBuffer = m_DebugDrawData.m_LineVertexBuffers[m_DebugDrawData.m_LineBatchDrawCallIndex]->GetPointer<LineVertexData>();

                for(auto& line : lines)
                {
                    if(m_DebugDrawData.LineIndexCount >= MaxLineIndices)
                        DebugLineFlush(pipeline);

                    m_DebugDrawData.m_LineBuffer->vertex = line.p1;
                    m_DebugDrawData.m_LineBuffer->colour = line.col;
                    m_DebugDrawData.m_LineBuffer->edge   = Vec4(0.0f); // 1px hairline, no SDF fade
                    m_DebugDrawData.m_LineBuffer++;

                    m_DebugDrawData.m_LineBuffer->vertex = line.p2;
                    m_DebugDrawData.m_LineBuffer->colour = line.col;
                    m_DebugDrawData.m_LineBuffer->edge   = Vec4(0.0f);
                    m_DebugDrawData.m_LineBuffer++;

                    m_DebugDrawData.LineIndexCount += 2;
                }

                // m_DebugDrawData.m_LineVertexBuffers[m_DebugDrawData.m_LineBatchDrawCallIndex]->ReleasePointer();
                // m_DebugDrawData.m_LineVertexBuffers[m_DebugDrawData.m_LineBatchDrawCallIndex]->Unbind();
                // m_DebugDrawData.m_LineVertexBuffers[m_DebugDrawData.m_LineBatchDrawCallIndex]->Bind(commandBuffer, pipeline.get());

                DebugLineFlush(pipeline);
            }

            if(!points.Empty())
            {
                LUMOS_PROFILE_SCOPE("Debug Points");
                m_DebugDrawData.m_PointDescriptorSet[0]->SetUniformBufferData(0, &projView);
                m_DebugDrawData.m_PointDescriptorSet[0]->Update();

                Graphics::CommandBuffer* commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();

                Graphics::PipelineDesc pipelineDesc;
                pipelineDesc.shader = m_DebugDrawData.m_PointShader;

                pipelineDesc.polygonMode         = Graphics::PolygonMode::FILL;
                pipelineDesc.cullMode            = Graphics::CullMode::BACK;
                pipelineDesc.transparencyEnabled = true;
                pipelineDesc.drawType            = DrawType::TRIANGLE;
                pipelineDesc.blendMode           = BlendMode::SrcAlphaOneMinusSrcAlpha;
                pipelineDesc.colourTargets[0]    = m_LastRenderTarget;
                pipelineDesc.DebugName           = "Debug-Points";
                pipelineDesc.samples = 1; // overlay: drawn after post-process onto the single-sample LDR target
                if(depthTest && m_MainTextureSamples <= 1)
                    pipelineDesc.depthTarget = m_ForwardData.m_DepthTexture;

                auto pipeline = Graphics::Pipeline::Get(pipelineDesc);
                commandBuffer->BindPipeline(pipeline);

                if(m_DebugDrawData.m_PointVertexBuffers.Empty())
                    m_DebugDrawData.m_PointVertexBuffers.EmplaceBack();

                int bufferCount = 0;
                bufferCount     = (int)m_DebugDrawData.m_PointVertexBuffers[0].Size();
                if((bufferCount - 1) < (int)m_DebugDrawData.m_PointBatchDrawCallIndex)
                {
                    m_DebugDrawData.m_PointVertexBuffers[0].EmplaceBack(Graphics::VertexBuffer::Create(RENDERER_POINT_BUFFER_SIZE, nullptr, BufferUsage::DYNAMIC));
                }

                m_DebugDrawData.m_PointVertexBuffers[0][m_DebugDrawData.m_PointBatchDrawCallIndex]->Bind(commandBuffer, pipeline.get());
                m_DebugDrawData.m_PointBuffer = m_PointBufferBase[0][0]; // m_DebugDrawData.m_PointVertexBuffers[m_DebugDrawData.m_PointBatchDrawCallIndex]->GetPointer<PointVertexData>();

                for(auto& pointInfo : points)
                {
                    if(m_DebugDrawData.PointIndexCount >= MaxPointIndices)
                        DebugPointFlush(pipeline);

                    Vec3 right = pointInfo.size * m_CameraTransform->GetRightDirection();
                    Vec3 up    = pointInfo.size * m_CameraTransform->GetUpDirection();

                    m_DebugDrawData.m_PointBuffer->vertex = pointInfo.p1 - right - up; // + Vec3(-pointInfo.size, -pointInfo.size, 0.0f));
                    m_DebugDrawData.m_PointBuffer->colour = pointInfo.col;
                    m_DebugDrawData.m_PointBuffer->size   = { pointInfo.size, 0.0f };
                    m_DebugDrawData.m_PointBuffer->uv     = { -1.0f, -1.0f };
                    m_DebugDrawData.m_PointBuffer++;

                    m_DebugDrawData.m_PointBuffer->vertex = pointInfo.p1 + right - up; //(pointInfo.p1 + Vec3(pointInfo.size, -pointInfo.size, 0.0f));
                    m_DebugDrawData.m_PointBuffer->colour = pointInfo.col;
                    m_DebugDrawData.m_PointBuffer->size   = { pointInfo.size, 0.0f };
                    m_DebugDrawData.m_PointBuffer->uv     = { 1.0f, -1.0f };
                    m_DebugDrawData.m_PointBuffer++;

                    m_DebugDrawData.m_PointBuffer->vertex = pointInfo.p1 + right + up; //(pointInfo.p1 + Vec3(pointInfo.size, pointInfo.size, 0.0f));
                    m_DebugDrawData.m_PointBuffer->colour = pointInfo.col;
                    m_DebugDrawData.m_PointBuffer->size   = { pointInfo.size, 0.0f };
                    m_DebugDrawData.m_PointBuffer->uv     = { 1.0f, 1.0f };
                    m_DebugDrawData.m_PointBuffer++;

                    m_DebugDrawData.m_PointBuffer->vertex = pointInfo.p1 - right + up; // (pointInfo.p1 + Vec3(-pointInfo.size, pointInfo.size, 0.0f));
                    m_DebugDrawData.m_PointBuffer->colour = pointInfo.col;
                    m_DebugDrawData.m_PointBuffer->size   = { pointInfo.size, 0.0f };
                    m_DebugDrawData.m_PointBuffer->uv     = { -1.0f, 1.0f };
                    m_DebugDrawData.m_PointBuffer++;

                    m_DebugDrawData.PointIndexCount += 6;
                }

                DebugPointFlush(pipeline.get());
            }

            if(!triangles.Empty())
            {
                LUMOS_PROFILE_SCOPE("Triangles Lines");

                m_DebugDrawData.m_Renderer2DData.m_DescriptorSet[0][0]->SetUniformBufferData(0, &projView);
                m_DebugDrawData.m_Renderer2DData.m_DescriptorSet[0][0]->Update();
                m_DebugDrawData.m_Renderer2DData.m_DescriptorSet[0][1]->Update();

                Graphics::PipelineDesc pipelineDesc;
                pipelineDesc.shader = m_DebugDrawData.m_Renderer2DData.m_Shader;

                pipelineDesc.polygonMode             = Graphics::PolygonMode::FILL;
                pipelineDesc.cullMode                = Graphics::CullMode::BACK;
                pipelineDesc.transparencyEnabled     = true;
                pipelineDesc.blendMode               = BlendMode::SrcAlphaOneMinusSrcAlpha;
                pipelineDesc.clearTargets            = false;
                pipelineDesc.colourTargets[0]        = m_LastRenderTarget;
                pipelineDesc.depthBiasEnabled        = true;
                pipelineDesc.depthBiasConstantFactor = 0.0f;
                pipelineDesc.depthBiasSlopeFactor    = -10.0f;
                pipelineDesc.DebugName               = "Debug-Triangles";
                pipelineDesc.samples = 1; // overlay: drawn after post-process onto the single-sample LDR target
                if(depthTest && m_MainTextureSamples <= 1)
                    pipelineDesc.depthTarget = m_ForwardData.m_DepthTexture;

                auto pipeline                                   = Graphics::Pipeline::Get(pipelineDesc);
                m_DebugDrawData.m_Renderer2DData.m_TextureCount = 0;
                uint32_t currentFrame                           = Renderer::GetMainSwapChain()->GetCurrentBufferIndex();

                if((int)m_DebugDrawData.m_Renderer2DData.m_VertexBuffers[currentFrame].Size() - 1 < (int)m_DebugDrawData.m_Renderer2DData.m_BatchDrawCallIndex)
                {
                    auto& vertexBuffer = m_DebugDrawData.m_Renderer2DData.m_VertexBuffers[currentFrame].EmplaceBack(Graphics::VertexBuffer::Create(BATCH_BUFFER_INITIAL, nullptr, BufferUsage::DYNAMIC));
                }

                m_DebugDrawData.m_Renderer2DData.m_VertexBuffers[currentFrame][m_DebugDrawData.m_Renderer2DData.m_BatchDrawCallIndex]->Bind(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), pipeline.get());
                // m_DebugDrawData.m_Renderer2DData.m_Buffer = m_DebugDrawData.m_Renderer2DData.m_VertexBuffers[currentFrame][m_DebugDrawData.m_Renderer2DData.m_BatchDrawCallIndex]->GetPointer<VertexData>();
                m_DebugDrawData.m_Renderer2DData.m_Buffer = m_QuadBufferBase[currentFrame][m_DebugDrawData.m_Renderer2DData.m_BatchDrawCallIndex];

                Arena* frameArena                  = Application::Get().GetFrameArena();
                DescriptorSet** currentDescriptors = PushArrayNoZero(frameArena, DescriptorSet*, 2);

                for(auto& triangleInfo : triangles)
                {
                    m_Stats.NumRenderedObjects++;

                    float textureSlot = 0.0f;

                    m_DebugDrawData.m_Renderer2DData.m_Buffer->vertex = triangleInfo.p1;
                    m_DebugDrawData.m_Renderer2DData.m_Buffer->uv     = { 0.0f, 0.0f, 0.0f, 0.0f };
                    m_DebugDrawData.m_Renderer2DData.m_Buffer->tid    = Vec2(textureSlot, 0.0f);
                    m_DebugDrawData.m_Renderer2DData.m_Buffer->colour = triangleInfo.col;
                    m_DebugDrawData.m_Renderer2DData.m_Buffer++;
                    m_DebugDrawData.m_Renderer2DData.m_Buffer->vertex = triangleInfo.p2;
                    m_DebugDrawData.m_Renderer2DData.m_Buffer->uv     = { 0.0f, 0.0f, 0.0f, 0.0f };
                    m_DebugDrawData.m_Renderer2DData.m_Buffer->tid    = Vec2(textureSlot, 0.0f);
                    m_DebugDrawData.m_Renderer2DData.m_Buffer->colour = triangleInfo.col;
                    m_DebugDrawData.m_Renderer2DData.m_Buffer++;
                    m_DebugDrawData.m_Renderer2DData.m_Buffer->vertex = triangleInfo.p3;
                    m_DebugDrawData.m_Renderer2DData.m_Buffer->uv     = { 0.0f, 0.0f, 0.0f, 0.0f };
                    m_DebugDrawData.m_Renderer2DData.m_Buffer->tid    = Vec2(textureSlot, 0.0f);
                    m_DebugDrawData.m_Renderer2DData.m_Buffer->colour = triangleInfo.col;
                    m_DebugDrawData.m_Renderer2DData.m_Buffer++;
                    m_DebugDrawData.m_Renderer2DData.m_IndexCount += 3;
                }

                Graphics::CommandBuffer* commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
                commandBuffer->UnBindPipeline();

                currentDescriptors[0] = m_DebugDrawData.m_Renderer2DData.m_DescriptorSet[0][0].get();
                currentDescriptors[1] = m_DebugDrawData.m_Renderer2DData.m_DescriptorSet[0][1].get();

                m_DebugDrawData.m_Renderer2DData.m_IndexBuffer->SetCount(m_DebugDrawData.m_Renderer2DData.m_IndexCount);
                m_DebugDrawData.m_Renderer2DData.m_IndexBuffer->Bind(commandBuffer);

                uint32_t dataSize = (uint32_t)((uint8_t*)m_DebugDrawData.m_Renderer2DData.m_Buffer - (uint8_t*)m_QuadBufferBase[currentFrame][m_DebugDrawData.m_Renderer2DData.m_BatchDrawCallIndex]);
                UploadBatch(m_DebugDrawData.m_Renderer2DData.m_VertexBuffers[currentFrame][m_DebugDrawData.m_Renderer2DData.m_BatchDrawCallIndex], dataSize, m_QuadBufferBase[currentFrame][m_DebugDrawData.m_Renderer2DData.m_BatchDrawCallIndex]);

                commandBuffer->BindPipeline(pipeline);

                Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, currentDescriptors, 2);
                Renderer::DrawIndexed(commandBuffer, DrawType::TRIANGLE, m_DebugDrawData.m_Renderer2DData.m_IndexCount);

                m_DebugDrawData.m_Renderer2DData.m_VertexBuffers[currentFrame][m_DebugDrawData.m_Renderer2DData.m_BatchDrawCallIndex]->Unbind();
                m_DebugDrawData.m_Renderer2DData.m_IndexBuffer->Unbind();

                // m_DebugDrawData.m_Renderer2DData.m_BatchDrawCallIndex++;
                m_DebugDrawData.m_Renderer2DData.m_Buffer     = m_QuadBufferBase[currentFrame][m_DebugDrawData.m_Renderer2DData.m_BatchDrawCallIndex];
                m_DebugDrawData.m_Renderer2DData.m_IndexCount = 0;
            }

            if(!thickLines.Empty())
            {
                LUMOS_PROFILE_SCOPE("Debug Thick Lines");

                m_DebugDrawData.m_LineDescriptorSet[0]->SetUniformBufferData(0, &projView);
                m_DebugDrawData.m_LineDescriptorSet[0]->Update();

                Graphics::PipelineDesc pipelineDesc;
                pipelineDesc.shader              = m_DebugDrawData.m_LineShader;
                pipelineDesc.polygonMode         = Graphics::PolygonMode::FILL;
                pipelineDesc.cullMode            = Graphics::CullMode::NONE;
                pipelineDesc.transparencyEnabled = true;
                pipelineDesc.clearTargets        = false;
                pipelineDesc.drawType            = DrawType::TRIANGLE;
                pipelineDesc.blendMode           = BlendMode::SrcAlphaOneMinusSrcAlpha;
                pipelineDesc.colourTargets[0]    = m_LastRenderTarget;
                // Pull toward camera so outlines don't z-fight (look faint) against coincident geometry.
                pipelineDesc.depthBiasEnabled        = true;
                pipelineDesc.depthBiasConstantFactor = -1.25f;
                pipelineDesc.depthBiasSlopeFactor    = -1.75f;
                pipelineDesc.DebugName           = "Debug-ThickLines";
                pipelineDesc.samples = 1; // overlay: drawn after post-process onto the single-sample LDR target
                if(depthTest && m_MainTextureSamples <= 1)
                    pipelineDesc.depthTarget = m_ForwardData.m_DepthTexture;

                auto pipeline = Graphics::Pipeline::Get(pipelineDesc);
                Renderer::GetMainSwapChain()->GetCurrentCommandBuffer()->BindPipeline(pipeline);

                if(m_DebugDrawData.m_LineVertexBuffers.Empty())
                    m_DebugDrawData.m_LineVertexBuffers.EmplaceBack();
                if((int)m_DebugDrawData.m_LineVertexBuffers[0].Size() - 1 < (int)m_DebugDrawData.m_LineBatchDrawCallIndex)
                    m_DebugDrawData.m_LineVertexBuffers[0].EmplaceBack(Graphics::VertexBuffer::Create(RENDERER_LINE_BUFFER_SIZE, nullptr, BufferUsage::DYNAMIC));

                m_DebugDrawData.m_LineVertexBuffers[0][m_DebugDrawData.m_LineBatchDrawCallIndex]->Bind(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), pipeline.get());
                m_DebugDrawData.m_LineBuffer = m_LineBufferBase[0][0];

                const Vec3 camPos = m_CameraTransform->GetWorldPosition();

                for(auto& line : thickLines)
                {
                    // 4 verts (6 indices) per quad; flush before overflowing the buffer.
                    if(m_DebugDrawData.LineIndexCount + 6 > MaxLineIndices)
                        DebugThickLineFlush(pipeline.get());

                    Vec3 dir = line.p2 - line.p1;
                    if(dir.LengthSquared() < 1e-12f)
                        continue;
                    dir.Normalise();

                    Vec3 toCam = camPos - (line.p1 + line.p2) * 0.5f;
                    Vec3 side  = dir.Cross(toCam);
                    if(side.LengthSquared() < 1e-12f)
                        continue; // line points straight at camera
                    side.Normalise();
                    side = side * (line.width * 0.5f);

                    const Vec3 v0 = line.p1 - side;
                    const Vec3 v1 = line.p1 + side;
                    const Vec3 v2 = line.p2 + side;
                    const Vec3 v3 = line.p2 - side;

                    const Vec3 tri[6]  = { v0, v1, v2, v2, v3, v0 };
                    const float edg[6] = { -1.0f, 1.0f, 1.0f, 1.0f, -1.0f, -1.0f };
                    for(int v = 0; v < 6; v++)
                    {
                        m_DebugDrawData.m_LineBuffer->vertex = tri[v];
                        m_DebugDrawData.m_LineBuffer->colour = line.col;
                        m_DebugDrawData.m_LineBuffer->edge   = Vec4(edg[v], 0.0f, 0.0f, 0.0f);
                        m_DebugDrawData.m_LineBuffer++;
                    }

                    m_DebugDrawData.LineIndexCount += 6;
                }

                DebugThickLineFlush(pipeline.get());
            }
        }

        m_DebugDrawData.m_PointBatchDrawCallIndex = 0;
        m_DebugDrawData.m_LineBatchDrawCallIndex  = 0;

        // Text Pass
        auto& dtDebugText = DebugRenderer::GetInstance()->GetDebugText();
        if(!dtDebugText.Empty())
        {
            LUMOS_PROFILE_SCOPE("Debug Text");

            Graphics::PipelineDesc pipelineDesc;
            pipelineDesc.shader              = m_DebugTextRendererData.m_Shader;
            pipelineDesc.polygonMode         = Graphics::PolygonMode::FILL;
            pipelineDesc.cullMode            = Graphics::CullMode::BACK;
            pipelineDesc.transparencyEnabled = true;
            pipelineDesc.blendMode           = BlendMode::SrcAlphaOneMinusSrcAlpha;
            pipelineDesc.clearTargets        = false;
            pipelineDesc.colourTargets[0]    = m_LastRenderTarget;
            pipelineDesc.DebugName           = "Debug-TextDT";
            pipelineDesc.samples             = 1; // overlay: single-sample LDR target after post-process
            m_DebugTextRendererData.m_Pipeline = Graphics::Pipeline::Get(pipelineDesc);

            uint32_t currentFrame = Renderer::GetMainSwapChain()->GetCurrentBufferIndex();

            while((int)m_DebugTextRendererData.m_VertexBuffers[currentFrame].Size() - 1 < (int)m_DebugTextRendererData.m_BatchDrawCallIndex)
            {
                auto& vertexBuffer = m_DebugTextRendererData.m_VertexBuffers[currentFrame].EmplaceBack(Graphics::VertexBuffer::Create(BufferUsage::DYNAMIC));
                vertexBuffer->Resize(RENDERER_LINE_BUFFER_SIZE);
            }

            m_DebugTextRendererData.m_VertexBuffers[currentFrame][m_DebugTextRendererData.m_BatchDrawCallIndex]->Bind(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), m_DebugTextRendererData.m_Pipeline.get());
            DebugTextVertexBufferPtr = DebugTextVertexBufferBase[currentFrame];
            auto projView            = m_Camera->GetGPUProjectionMatrix() * Mat4::Inverse(m_CameraTransform->GetWorldMatrix());

            m_DebugTextRendererData.m_DescriptorSet[m_DebugTextRendererData.m_BatchDrawCallIndex][0]->SetUniformBufferData(0, &projView);
            m_DebugTextRendererData.m_DescriptorSet[m_DebugTextRendererData.m_BatchDrawCallIndex][0]->Update();

            m_DebugTextRendererData.m_TextureCount = 0;
            m_Stats.NumRenderedObjects++;

            if(m_DebugTextRendererData.m_IndexCount >= m_DebugTextRendererData.m_Limits.IndiciesSize)
                TextFlush(m_DebugTextRendererData, DebugTextVertexBufferBase, DebugTextVertexBufferPtr);

            int textureIndex = -1;
            // auto& string = textComp.TextString;
            auto font              = Font::GetDefaultFont();
            float lineHeightOffset = 0.0f;
            float kerningOffset    = 0.0f;
            float outlineWidth     = 0.0f;

            SharedPtr<Texture2D> fontAtlas = font->GetFontAtlas();
            if(!fontAtlas)
                return; // TODO

            for(uint32_t i = 0; i < m_DebugTextRendererData.m_TextureCount; i++)
            {
                if(m_DebugTextRendererData.m_Textures[i] == fontAtlas.get())
                {
                    textureIndex = int(i);
                    break;
                }
            }

            if(textureIndex == -1)
            {
                textureIndex                                                               = (int)m_DebugTextRendererData.m_TextureCount;
                m_DebugTextRendererData.m_Textures[m_DebugTextRendererData.m_TextureCount] = fontAtlas.get();
                m_DebugTextRendererData.m_TextureCount++;
            }

            auto& fontGeometry  = font->GetMSDFData()->FontGeometry;
            const auto& metrics = fontGeometry.getMetrics();

            for(auto& text : dtDebugText)
            {
                double x       = 0.0;
                double fsScale = 1 / (metrics.ascenderY - metrics.descenderY);
                double y       = 0.0;
                for(int i = 0; i < text.text.size(); i++)
                {
                    UnicodeDecode dec  = Utf8Decode((u8*)text.text.data() + i, text.text.size() - i);
                    char32_t character = dec.codepoint;
                    if(dec.inc > 1)
                        i += dec.inc - 1;
                    Vec4 pos           = text.Position;
                    Vec4 colour        = text.colour;
                    Vec4 outlineColour = text.colour;
                    float size         = text.Size;

                    if(character == '\r')
                        continue;

                    if(character == '\n')
                    {
                        x = 0;
                        y -= fsScale * metrics.lineHeight + lineHeightOffset;
                        continue;
                    }

                    if(character == '\t')
                    {
                        auto glyph     = fontGeometry.getGlyph('a');
                        double advance = glyph->getAdvance();
                        x += 4 * fsScale * advance + kerningOffset;
                        continue;
                    }

                    auto glyph = font->GetMSDFData()->GetGlyph(character);
                    if(!glyph)
                        glyph = fontGeometry.getGlyph('?');
                    if(!glyph)
                        continue;

                    double l, b, r, t;
                    glyph->getQuadAtlasBounds(l, b, r, t);

                    double pl, pb, pr, pt;
                    glyph->getQuadPlaneBounds(pl, pb, pr, pt);

                    pl *= fsScale, pb *= fsScale, pr *= fsScale, pt *= fsScale;
                    pl += x, pb += y, pr += x, pt += y;

                    double texelWidth  = 1. / fontAtlas->GetWidth();
                    double texelHeight = 1. / fontAtlas->GetHeight();
                    l *= texelWidth, b *= texelHeight, r *= texelWidth, t *= texelHeight;

                    Mat4 transform = Mat4::Translation(Vec3(pos)) * Maths::ToMat4(m_CameraTransform->GetLocalOrientation()) * Mat4::Scale(Vec3(size / 10.0f));

                    {
                        LUMOS_PROFILE_SCOPE("Set text buffer data");
                        DebugTextVertexBufferPtr->vertex        = transform * Vec4((float)pl, (float)pb, 0.0f, 1.0f);
                        DebugTextVertexBufferPtr->colour        = colour;
                        DebugTextVertexBufferPtr->uv            = { (float)l, (float)b };
                        DebugTextVertexBufferPtr->tid           = Vec2((float)textureIndex, outlineWidth);
                        DebugTextVertexBufferPtr->outlineColour = outlineColour;
                        DebugTextVertexBufferPtr++;

                        DebugTextVertexBufferPtr->vertex        = transform * Vec4((float)pr, (float)pb, 0.0f, 1.0f);
                        DebugTextVertexBufferPtr->colour        = colour;
                        DebugTextVertexBufferPtr->uv            = { (float)r, (float)b };
                        DebugTextVertexBufferPtr->tid           = Vec2((float)textureIndex, outlineWidth);
                        DebugTextVertexBufferPtr->outlineColour = outlineColour;
                        DebugTextVertexBufferPtr++;

                        DebugTextVertexBufferPtr->vertex        = transform * Vec4((float)pr, (float)pt, 0.0f, 1.0f);
                        DebugTextVertexBufferPtr->colour        = colour;
                        DebugTextVertexBufferPtr->uv            = { (float)r, (float)t };
                        DebugTextVertexBufferPtr->tid           = Vec2((float)textureIndex, outlineWidth);
                        DebugTextVertexBufferPtr->outlineColour = outlineColour;
                        DebugTextVertexBufferPtr++;

                        DebugTextVertexBufferPtr->vertex        = transform * Vec4((float)pl, (float)pt, 0.0f, 1.0f);
                        DebugTextVertexBufferPtr->colour        = colour;
                        DebugTextVertexBufferPtr->uv            = { (float)l, (float)t };
                        DebugTextVertexBufferPtr->tid           = Vec2((float)textureIndex, outlineWidth);
                        DebugTextVertexBufferPtr->outlineColour = outlineColour;
                        DebugTextVertexBufferPtr++;
                    }

                    m_DebugTextRendererData.m_IndexCount += 6;

                    double advance = glyph->getAdvance();

                    char32_t next = (i + 1 < (int)text.text.size()) ? Utf8Decode((u8*)text.text.data() + i + 1, text.text.size() - i - 1).codepoint : 0;
                    fontGeometry.getAdvance(advance, character, next); // next codepoint, not next raw byte
                    x += fsScale * advance + kerningOffset;
                }
            }

            if(m_DebugTextRendererData.m_IndexCount != 0)
                TextFlush(m_DebugTextRendererData, DebugTextVertexBufferBase, DebugTextVertexBufferPtr);
        }

        auto& ndtDebugText = DebugRenderer::GetInstance()->GetDebugTextNDT();
        if(!ndtDebugText.Empty())
        {
            LUMOS_PROFILE_SCOPE("NDT Debug Lines");

            Graphics::PipelineDesc pipelineDesc;
            pipelineDesc.shader              = m_DebugTextRendererData.m_Shader;
            pipelineDesc.polygonMode         = Graphics::PolygonMode::FILL;
            pipelineDesc.cullMode            = Graphics::CullMode::BACK;
            pipelineDesc.transparencyEnabled = true;
            pipelineDesc.blendMode           = BlendMode::SrcAlphaOneMinusSrcAlpha;
            pipelineDesc.clearTargets        = false;
            pipelineDesc.colourTargets[0]    = m_MainTexture;
            pipelineDesc.DebugName           = "Debug-TextNDT";
            if(m_MainTextureSamples > 1)
                pipelineDesc.resolveTexture = m_ResolveTexture;
            pipelineDesc.samples               = m_MainTextureSamples;
            m_DebugTextRendererData.m_Pipeline = Graphics::Pipeline::Get(pipelineDesc);

            uint32_t currentFrame = Renderer::GetMainSwapChain()->GetCurrentBufferIndex();

            while((int)m_DebugTextRendererData.m_VertexBuffers[currentFrame].Size() - 1 < (int)m_DebugTextRendererData.m_BatchDrawCallIndex)
            {
                auto& vertexBuffer = m_DebugTextRendererData.m_VertexBuffers[currentFrame].EmplaceBack(Graphics::VertexBuffer::Create(BufferUsage::DYNAMIC));
                vertexBuffer->Resize(RENDERER_LINE_BUFFER_SIZE);
            }

            m_DebugTextRendererData.m_VertexBuffers[currentFrame][m_DebugTextRendererData.m_BatchDrawCallIndex]->Bind(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), m_DebugTextRendererData.m_Pipeline.get());
            DebugTextVertexBufferPtr = DebugTextVertexBufferBase[currentFrame];
            auto projView            = m_Camera->GetGPUProjectionMatrix() * Mat4::Inverse(m_CameraTransform->GetWorldMatrix());
            DebugRenderer::GetInstance()->SetProjView(projView);
            DebugRenderer::SortLists();

            m_DebugTextRendererData.m_DescriptorSet[m_DebugTextRendererData.m_BatchDrawCallIndex][0]->SetUniformBufferData(0, &projView);
            m_DebugTextRendererData.m_DescriptorSet[m_DebugTextRendererData.m_BatchDrawCallIndex][0]->Update();

            m_DebugTextRendererData.m_TextureCount = 0;
            m_Stats.NumRenderedObjects++;

            if(m_DebugTextRendererData.m_IndexCount >= m_DebugTextRendererData.m_Limits.IndiciesSize)
                TextFlush(m_DebugTextRendererData, DebugTextVertexBufferBase, DebugTextVertexBufferPtr);

            int textureIndex = -1;
            // auto& string = textComp.TextString;
            auto font              = Font::GetDefaultFont();
            float lineHeightOffset = 0.0f;
            float kerningOffset    = 0.0f;
            float outlineWidth     = 0.0f;

            SharedPtr<Texture2D> fontAtlas = font->GetFontAtlas();
            if(!fontAtlas)
                return; // TODO

            for(uint32_t i = 0; i < m_DebugTextRendererData.m_TextureCount; i++)
            {
                if(m_DebugTextRendererData.m_Textures[i] == fontAtlas.get())
                {
                    textureIndex = int(i);
                    break;
                }
            }

            if(textureIndex == -1)
            {
                textureIndex                                                               = (int)m_DebugTextRendererData.m_TextureCount;
                m_DebugTextRendererData.m_Textures[m_DebugTextRendererData.m_TextureCount] = fontAtlas.get();
                m_DebugTextRendererData.m_TextureCount++;
            }

            auto& fontGeometry  = font->GetMSDFData()->FontGeometry;
            const auto& metrics = fontGeometry.getMetrics();

            for(auto& text : ndtDebugText)
            {
                double x       = 0.0;
                double fsScale = 1 / (metrics.ascenderY - metrics.descenderY);
                double y       = 0.0;
                for(int i = 0; i < text.text.size(); i++)
                {
                    UnicodeDecode dec  = Utf8Decode((u8*)text.text.data() + i, text.text.size() - i);
                    char32_t character = dec.codepoint;
                    if(dec.inc > 1)
                        i += dec.inc - 1;
                    Vec4 pos           = text.Position;
                    Vec4 colour        = text.colour;
                    Vec4 outlineColour = text.colour;
                    float size         = text.Size;
                    if(character == '\r')
                        continue;

                    if(character == '\n')
                    {
                        x = 0;
                        y -= fsScale * metrics.lineHeight + lineHeightOffset;
                        continue;
                    }

                    if(character == '\t')
                    {
                        auto glyph     = fontGeometry.getGlyph('a');
                        double advance = glyph->getAdvance();
                        x += 4 * fsScale * advance + kerningOffset;
                        continue;
                    }

                    auto glyph = font->GetMSDFData()->GetGlyph(character);
                    if(!glyph)
                        glyph = fontGeometry.getGlyph('?');
                    if(!glyph)
                        continue;

                    double l, b, r, t;
                    glyph->getQuadAtlasBounds(l, b, r, t);

                    double pl, pb, pr, pt;
                    glyph->getQuadPlaneBounds(pl, pb, pr, pt);

                    pl *= fsScale, pb *= fsScale, pr *= fsScale, pt *= fsScale;
                    pl += x, pb += y, pr += x, pt += y;

                    double texelWidth  = 1. / fontAtlas->GetWidth();
                    double texelHeight = 1. / fontAtlas->GetHeight();
                    l *= texelWidth, b *= texelHeight, r *= texelWidth, t *= texelHeight;

                    Mat4 transform = Mat4::Translation(Vec3(pos)) * Maths::ToMat4(m_CameraTransform->GetLocalOrientation()) * Mat4::Scale(Vec3(size / 10.0f));

                    {
                        LUMOS_PROFILE_SCOPE("Set text buffer data");
                        DebugTextVertexBufferPtr->vertex        = transform * Vec4((float)pl, (float)pb, 0.0f, 1.0f);
                        DebugTextVertexBufferPtr->colour        = colour;
                        DebugTextVertexBufferPtr->uv            = { (float)l, (float)b };
                        DebugTextVertexBufferPtr->tid           = Vec2((float)textureIndex, outlineWidth);
                        DebugTextVertexBufferPtr->outlineColour = outlineColour;
                        DebugTextVertexBufferPtr++;

                        DebugTextVertexBufferPtr->vertex        = transform * Vec4((float)pr, (float)pb, 0.0f, 1.0f);
                        DebugTextVertexBufferPtr->colour        = colour;
                        DebugTextVertexBufferPtr->uv            = { (float)r, (float)b };
                        DebugTextVertexBufferPtr->tid           = Vec2((float)textureIndex, outlineWidth);
                        DebugTextVertexBufferPtr->outlineColour = outlineColour;
                        DebugTextVertexBufferPtr++;

                        DebugTextVertexBufferPtr->vertex        = transform * Vec4((float)pr, (float)pt, 0.0f, 1.0f);
                        DebugTextVertexBufferPtr->colour        = colour;
                        DebugTextVertexBufferPtr->uv            = { (float)r, (float)t };
                        DebugTextVertexBufferPtr->tid           = Vec2((float)textureIndex, outlineWidth);
                        DebugTextVertexBufferPtr->outlineColour = outlineColour;
                        DebugTextVertexBufferPtr++;

                        DebugTextVertexBufferPtr->vertex        = transform * Vec4((float)pl, (float)pt, 0.0f, 1.0f);
                        DebugTextVertexBufferPtr->colour        = colour;
                        DebugTextVertexBufferPtr->uv            = { (float)l, (float)t };
                        DebugTextVertexBufferPtr->tid           = Vec2((float)textureIndex, outlineWidth);
                        DebugTextVertexBufferPtr->outlineColour = outlineColour;
                        DebugTextVertexBufferPtr++;
                    }

                    m_DebugTextRendererData.m_IndexCount += 6;

                    double advance = glyph->getAdvance();

                    char32_t next = (i + 1 < (int)text.text.size()) ? Utf8Decode((u8*)text.text.data() + i + 1, text.text.size() - i - 1).codepoint : 0;
                    fontGeometry.getAdvance(advance, character, next); // next codepoint, not next raw byte
                    x += fsScale * advance + kerningOffset;
                }
            }

            if(m_DebugTextRendererData.m_IndexCount != 0)
                TextFlush(m_DebugTextRendererData, DebugTextVertexBufferBase, DebugTextVertexBufferPtr);
        }

        Mat4 csProjection = ReverseZ(Mat4::Orthographic(0.0f, (float)m_MainTexture->GetWidth(), 0.0f, (float)m_MainTexture->GetHeight(), -100.0f, 100.0f));
        auto projView     = m_Camera->GetProjectionMatrix();
        projView          = Mat4(1.0f);

        float scale       = 1.0f;
        float aspectRatio = (float)m_MainTexture->GetWidth() / (float)m_MainTexture->GetHeight();
        projView          = ReverseZ(Mat4::Orthographic(-aspectRatio * scale, aspectRatio * scale, -scale, scale, -10.0f, 10.0f));

        DebugRenderer::GetInstance()->SetProjView(projView);
        DebugRenderer::SortLists();

        auto& csDebugText = DebugRenderer::GetInstance()->GetDebugTextCS();
        if(!csDebugText.Empty())
        {
            LUMOS_PROFILE_SCOPE("CS Debug Lines");

            Graphics::PipelineDesc pipelineDesc;
            pipelineDesc.shader              = m_DebugTextRendererData.m_Shader;
            pipelineDesc.polygonMode         = Graphics::PolygonMode::FILL;
            pipelineDesc.cullMode            = Graphics::CullMode::BACK;
            pipelineDesc.transparencyEnabled = true;
            pipelineDesc.blendMode           = BlendMode::SrcAlphaOneMinusSrcAlpha;
            pipelineDesc.clearTargets        = false;
            pipelineDesc.colourTargets[0]    = m_MainTexture;
            pipelineDesc.DebugName           = "Debug-TextCS";
            if(m_MainTextureSamples > 1)
                pipelineDesc.resolveTexture = m_ResolveTexture;
            pipelineDesc.samples               = m_MainTextureSamples;
            m_DebugTextRendererData.m_Pipeline = Graphics::Pipeline::Get(pipelineDesc);

            uint32_t currentFrame = Renderer::GetMainSwapChain()->GetCurrentBufferIndex();

            while((int)m_DebugTextRendererData.m_VertexBuffers[currentFrame].Size() - 1 < (int)m_DebugTextRendererData.m_BatchDrawCallIndex)
            {
                auto& vertexBuffer = m_DebugTextRendererData.m_VertexBuffers[currentFrame].EmplaceBack(Graphics::VertexBuffer::Create(BufferUsage::DYNAMIC));
                vertexBuffer->Resize(RENDERER_LINE_BUFFER_SIZE);
            }

            m_DebugTextRendererData.m_VertexBuffers[currentFrame][m_DebugTextRendererData.m_BatchDrawCallIndex]->Bind(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), m_DebugTextRendererData.m_Pipeline.get());
            DebugTextVertexBufferPtr = DebugTextVertexBufferBase[currentFrame];

            m_DebugTextRendererData.m_DescriptorSet[m_DebugTextRendererData.m_BatchDrawCallIndex][0]->SetUniformBufferData(0, &projView);
            m_DebugTextRendererData.m_DescriptorSet[m_DebugTextRendererData.m_BatchDrawCallIndex][0]->Update();

            m_DebugTextRendererData.m_TextureCount = 0;
            m_Stats.NumRenderedObjects++;

            if(m_DebugTextRendererData.m_IndexCount >= m_DebugTextRendererData.m_Limits.IndiciesSize)
                TextFlush(m_DebugTextRendererData, DebugTextVertexBufferBase, DebugTextVertexBufferPtr);

            int textureIndex = -1;
            // auto& string = textComp.TextString;
            auto font              = Font::GetDefaultFont();
            float lineHeightOffset = 0.0f;
            float kerningOffset    = 0.0f;
            float outlineWidth     = 0.0f;

            SharedPtr<Texture2D> fontAtlas = font->GetFontAtlas();
            if(!fontAtlas)
                return; // TODO

            for(uint32_t i = 0; i < m_DebugTextRendererData.m_TextureCount; i++)
            {
                if(m_DebugTextRendererData.m_Textures[i] == fontAtlas.get())
                {
                    textureIndex = int(i);
                    break;
                }
            }

            if(textureIndex == -1)
            {
                textureIndex                                                               = (int)m_DebugTextRendererData.m_TextureCount;
                m_DebugTextRendererData.m_Textures[m_DebugTextRendererData.m_TextureCount] = fontAtlas.get();
                m_DebugTextRendererData.m_TextureCount++;
            }

            auto& fontGeometry  = font->GetMSDFData()->FontGeometry;
            const auto& metrics = fontGeometry.getMetrics();

            for(auto& text : csDebugText)
            {
                double x       = 0.0;
                double fsScale = 1 / (metrics.ascenderY - metrics.descenderY);
                double y       = 0.0;
                for(int i = 0; i < text.text.size(); i++)
                {
                    UnicodeDecode dec  = Utf8Decode((u8*)text.text.data() + i, text.text.size() - i);
                    char32_t character = dec.codepoint;
                    if(dec.inc > 1)
                        i += dec.inc - 1;
                    Vec4 pos           = text.Position;
                    Vec4 colour        = text.colour;
                    Vec4 outlineColour = text.colour;

                    if(character == '\r')
                        continue;

                    if(character == '\n')
                    {
                        x = 0;
                        y -= fsScale * metrics.lineHeight + lineHeightOffset;
                        continue;
                    }

                    if(character == '\t')
                    {
                        auto glyph     = fontGeometry.getGlyph('a');
                        double advance = glyph->getAdvance();
                        x += 4 * fsScale * advance + kerningOffset;
                        continue;
                    }

                    auto glyph = font->GetMSDFData()->GetGlyph(character);
                    if(!glyph)
                        glyph = fontGeometry.getGlyph('?');
                    if(!glyph)
                        continue;

                    double l, b, r, t;
                    glyph->getQuadAtlasBounds(l, b, r, t);

                    double pl, pb, pr, pt;
                    glyph->getQuadPlaneBounds(pl, pb, pr, pt);

                    pl *= fsScale, pb *= fsScale, pr *= fsScale, pt *= fsScale;
                    pl += x, pb += y, pr += x, pt += y;

                    double texelWidth  = 1. / fontAtlas->GetWidth();
                    double texelHeight = 1. / fontAtlas->GetHeight();
                    l *= texelWidth, b *= texelHeight, r *= texelWidth, t *= texelHeight;

                    Mat4 transform = Mat4::Translation(Vec3(pos)) * Mat4::Scale(Vec3(0.05f, 0.05f, 0.05f));

                    {
                        LUMOS_PROFILE_SCOPE("Set text buffer data");
                        DebugTextVertexBufferPtr->vertex        = transform * Vec4((float)pl, (float)pb, 0.0f, 1.0f);
                        DebugTextVertexBufferPtr->colour        = colour;
                        DebugTextVertexBufferPtr->uv            = { (float)l, (float)b };
                        DebugTextVertexBufferPtr->tid           = Vec2((float)textureIndex, outlineWidth);
                        DebugTextVertexBufferPtr->outlineColour = outlineColour;
                        DebugTextVertexBufferPtr++;

                        DebugTextVertexBufferPtr->vertex        = transform * Vec4((float)pr, (float)pb, 0.0f, 1.0f);
                        DebugTextVertexBufferPtr->colour        = colour;
                        DebugTextVertexBufferPtr->uv            = { (float)r, (float)b };
                        DebugTextVertexBufferPtr->tid           = Vec2((float)textureIndex, outlineWidth);
                        DebugTextVertexBufferPtr->outlineColour = outlineColour;
                        DebugTextVertexBufferPtr++;

                        DebugTextVertexBufferPtr->vertex        = transform * Vec4((float)pr, (float)pt, 0.0f, 1.0f);
                        DebugTextVertexBufferPtr->colour        = colour;
                        DebugTextVertexBufferPtr->uv            = { (float)r, (float)t };
                        DebugTextVertexBufferPtr->tid           = Vec2((float)textureIndex, outlineWidth);
                        DebugTextVertexBufferPtr->outlineColour = outlineColour;
                        DebugTextVertexBufferPtr++;

                        DebugTextVertexBufferPtr->vertex        = transform * Vec4((float)pl, (float)pt, 0.0f, 1.0f);
                        DebugTextVertexBufferPtr->colour        = colour;
                        DebugTextVertexBufferPtr->uv            = { (float)l, (float)t };
                        DebugTextVertexBufferPtr->tid           = Vec2((float)textureIndex, outlineWidth);
                        DebugTextVertexBufferPtr->outlineColour = outlineColour;
                        DebugTextVertexBufferPtr++;
                    }

                    m_DebugTextRendererData.m_IndexCount += 6;

                    double advance = glyph->getAdvance();

                    char32_t next = (i + 1 < (int)text.text.size()) ? Utf8Decode((u8*)text.text.data() + i + 1, text.text.size() - i - 1).codepoint : 0;
                    fontGeometry.getAdvance(advance, character, next); // next codepoint, not next raw byte
                    x += fsScale * advance + kerningOffset;
                }
            }

            if(m_DebugTextRendererData.m_IndexCount != 0)
                TextFlush(m_DebugTextRendererData, DebugTextVertexBufferBase, DebugTextVertexBufferPtr);
        }
    }

    void SceneRenderer::DebugLineFlush(Graphics::Pipeline* pipeline)
    {
        LUMOS_PROFILE_FUNCTION();

        m_DebugDrawData.m_LineIndexBuffer->SetCount(m_DebugDrawData.LineIndexCount);
        m_DebugDrawData.m_LineIndexBuffer->Bind(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer());

        uint32_t dataSize = (uint32_t)((uint8_t*)m_DebugDrawData.m_LineBuffer - (uint8_t*)m_LineBufferBase[0][0]);
        m_DebugDrawData.m_LineVertexBuffers[0][m_DebugDrawData.m_LineBatchDrawCallIndex]->SetData(dataSize, (void*)m_LineBufferBase[0][0]);
        // SetData may have grown the buffer into a new one: what was bound before is gone
        m_DebugDrawData.m_LineVertexBuffers[0][m_DebugDrawData.m_LineBatchDrawCallIndex]->Bind(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), pipeline);

        auto* desc = m_DebugDrawData.m_LineDescriptorSet[0].get();
        Renderer::BindDescriptorSets(pipeline, Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), 0, &desc, 1);
        Renderer::DrawIndexed(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), DrawType::LINES, m_DebugDrawData.LineIndexCount);

        m_DebugDrawData.m_LineVertexBuffers[0][m_DebugDrawData.m_LineBatchDrawCallIndex]->Unbind();
        m_DebugDrawData.m_LineIndexBuffer->Unbind();

        m_DebugDrawData.m_LineBatchDrawCallIndex++;
        m_DebugDrawData.m_LineBuffer   = m_LineBufferBase[0][0]; //[m_DebugDrawData.m_LineBatchDrawCallIndex];
        m_DebugDrawData.LineIndexCount = 0;

        if(m_DebugDrawData.m_LineVertexBuffers.Empty())
            m_DebugDrawData.m_LineVertexBuffers.EmplaceBack();
        if((int)m_DebugDrawData.m_LineVertexBuffers[0].Size() - 1 < (int)m_DebugDrawData.m_LineBatchDrawCallIndex)
        {
            auto& vertexBuffer = m_DebugDrawData.m_LineVertexBuffers[0].EmplaceBack(Graphics::VertexBuffer::Create(RENDERER_LINE_BUFFER_SIZE, nullptr, BufferUsage::DYNAMIC));
        }

        m_DebugDrawData.m_LineVertexBuffers[0][m_DebugDrawData.m_LineBatchDrawCallIndex]->Bind(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), pipeline);
        m_DebugDrawData.m_LineBuffer   = m_LineBufferBase[0][0]; //[m_DebugDrawData.m_LineBatchDrawCallIndex];
        m_DebugDrawData.LineIndexCount = 0;
    }

    void SceneRenderer::DebugThickLineFlush(Graphics::Pipeline* pipeline)
    {
        LUMOS_PROFILE_FUNCTION();

        if(m_DebugDrawData.LineIndexCount == 0)
            return;

        Graphics::IndexBuffer* lineIndices = m_DebugDrawData.m_LineIndexBuffer;
        lineIndices->SetCount(m_DebugDrawData.LineIndexCount);
        lineIndices->Bind(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer());

        uint32_t dataSize = (uint32_t)((uint8_t*)m_DebugDrawData.m_LineBuffer - (uint8_t*)m_LineBufferBase[0][0]);
        m_DebugDrawData.m_LineVertexBuffers[0][m_DebugDrawData.m_LineBatchDrawCallIndex]->SetData(dataSize, (void*)m_LineBufferBase[0][0]);

        auto* desc = m_DebugDrawData.m_LineDescriptorSet[0].get();
        Renderer::BindDescriptorSets(pipeline, Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), 0, &desc, 1);
        Renderer::DrawIndexed(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), DrawType::TRIANGLE, m_DebugDrawData.LineIndexCount);

        m_DebugDrawData.m_LineVertexBuffers[0][m_DebugDrawData.m_LineBatchDrawCallIndex]->Unbind();
        lineIndices->Unbind();

        m_DebugDrawData.m_LineBatchDrawCallIndex++;
        m_DebugDrawData.LineIndexCount = 0;

        if(m_DebugDrawData.m_LineVertexBuffers.Empty())
            m_DebugDrawData.m_LineVertexBuffers.EmplaceBack();
        if((int)m_DebugDrawData.m_LineVertexBuffers[0].Size() - 1 < (int)m_DebugDrawData.m_LineBatchDrawCallIndex)
            m_DebugDrawData.m_LineVertexBuffers[0].EmplaceBack(Graphics::VertexBuffer::Create(RENDERER_LINE_BUFFER_SIZE, nullptr, BufferUsage::DYNAMIC));

        m_DebugDrawData.m_LineVertexBuffers[0][m_DebugDrawData.m_LineBatchDrawCallIndex]->Bind(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), pipeline);
        m_DebugDrawData.m_LineBuffer   = m_LineBufferBase[0][0];
        m_DebugDrawData.LineIndexCount = 0;
    }

    void SceneRenderer::DebugPointFlush(Graphics::Pipeline* pipeline)
    {
        LUMOS_PROFILE_FUNCTION();

        // m_DebugDrawData.m_PointVertexBuffers[m_DebugDrawData.m_PointBatchDrawCallIndex]->ReleasePointer();
        m_DebugDrawData.m_PointIndexBuffer->SetCount(m_DebugDrawData.PointIndexCount);
        m_DebugDrawData.m_PointIndexBuffer->Bind(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer());

        uint32_t dataSize = (uint32_t)((uint8_t*)m_DebugDrawData.m_PointBuffer - (uint8_t*)m_PointBufferBase[0][0]);
        m_DebugDrawData.m_PointVertexBuffers[0][m_DebugDrawData.m_PointBatchDrawCallIndex]->SetData(dataSize, (void*)m_PointBufferBase[0][0]);

        auto* desc = m_DebugDrawData.m_PointDescriptorSet[0].get();
        Renderer::BindDescriptorSets(pipeline, Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), 0, &desc, 1);
        Renderer::DrawIndexed(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), DrawType::TRIANGLE, m_DebugDrawData.PointIndexCount);

        m_DebugDrawData.m_PointVertexBuffers[0][m_DebugDrawData.m_PointBatchDrawCallIndex]->Unbind();
        m_DebugDrawData.m_PointIndexBuffer->Unbind();

        m_DebugDrawData.m_PointBatchDrawCallIndex++;

        if(m_DebugDrawData.m_PointVertexBuffers.Empty())
            m_DebugDrawData.m_PointVertexBuffers.EmplaceBack();

        if((int)m_DebugDrawData.m_PointVertexBuffers[0].Size() - 1 < (int)m_DebugDrawData.m_PointBatchDrawCallIndex)
        {
            m_DebugDrawData.m_PointVertexBuffers[0].EmplaceBack(Graphics::VertexBuffer::Create(RENDERER_POINT_BUFFER_SIZE, nullptr, BufferUsage::DYNAMIC));
        }

        m_DebugDrawData.m_PointVertexBuffers[0][m_DebugDrawData.m_PointBatchDrawCallIndex]->Bind(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), pipeline);
        m_DebugDrawData.m_PointBuffer   = m_PointBufferBase[0][0];
        m_DebugDrawData.PointIndexCount = 0;
    }

    int SceneRenderer::CaptureEnvironmentProbe(const Vec3& pos, u32 size, float gain, int index)
    {
        if(index < 0 || index >= (int)m_Probes.Size())
        {
            index = (int)m_Probes.Size();
            m_Probes.EmplaceBack();
        }
        m_Probes[index].Position = pos;
        m_Probe.Index         = index;
        m_Probe.Position      = pos;
        m_Probe.Gain          = gain;
        m_Probe.Size          = Maths::Max(16u, size);
        m_Probe.Face          = 0;
        m_Probe.PendingFilter = false;
        if(!m_Probe.Cam)
        {
            m_Probe.Cam   = new Camera(100.0f, 0.05f, 400.0f, 1.0f);
            m_Probe.Trans = new Maths::Transform();
        }
        return index;
    }

    void SceneRenderer::ClearEnvironmentProbes()
    {
        m_Probe.Face          = -1;
        m_Probe.PendingFilter = false;
        m_Probes.Clear();
        m_ActiveProbe                  = -1;
        m_ForwardData.m_EnvironmentMap = nullptr; // the Environment's maps go back on next frame
    }

    // The probe camera's frame is in the main texture: copy the directions of this cube face out of it.
    void SceneRenderer::ProbeFacePass()
    {
        if(m_Probe.Face < 0)
            return;
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("Probe Face Pass");

        auto shader = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("EnvironmentProbeFace")).As<Graphics::Shader>();
        if(!shader)
        {
            m_Probe.Face = -1;
            return;
        }
        if(!m_Probe.Cube || m_Probe.Cube->GetWidth() != m_Probe.Size)
            m_Probe.Cube = SharedPtr<TextureCube>(TextureCube::Create(m_Probe.Size, nullptr, true));
        if(!m_Probe.Set)
        {
            Graphics::DescriptorDesc desc {};
            desc.layoutIndex = 0;
            desc.shader      = shader.get();
            m_Probe.Set      = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(desc));
        }

        auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
        commandBuffer->UnBindPipeline();

        struct
        {
            Mat4 ProjView;
            Vec4 Position;
        } ubo = { m_Probe.ProjView, Vec4(m_Probe.Position, m_Probe.Gain) };
        Texture* colour = m_MainTextureSamples > 1 ? (Texture*)m_ResolveTexture : (Texture*)m_MainTexture;
        m_Probe.Set->SetTexture(0, colour);
        m_Probe.Set->SetUniformBufferData(1, &ubo);
        m_Probe.Set->Update(commandBuffer);
        m_Probe.Set->TransitionImages(commandBuffer);

        Graphics::PipelineDesc pipelineDesc {};
        pipelineDesc.shader              = shader;
        pipelineDesc.polygonMode         = Graphics::PolygonMode::FILL;
        pipelineDesc.cullMode            = Graphics::CullMode::BACK;
        pipelineDesc.transparencyEnabled = false;
        pipelineDesc.cubeMapTarget       = m_Probe.Cube.get();
        pipelineDesc.cubeMapIndex        = (u32)m_Probe.Face;
        pipelineDesc.DebugName           = "Probe Face";
        auto pipeline                    = Graphics::Pipeline::Get(pipelineDesc);
        commandBuffer->BindPipeline(pipeline, (u32)m_Probe.Face);

        u32 face = (u32)m_Probe.Face;
        auto& pushConstants = shader->GetPushConstants();
        if(!pushConstants.Empty())
        {
            pushConstants[0].SetValue("cubeFaceIndex", (void*)&face);
            shader->BindPushConstants(commandBuffer, pipeline.get());
        }
        auto set = m_Probe.Set.get();
        Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
        Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);
        commandBuffer->UnBindPipeline();
        commandBuffer->EndCurrentRenderPass();

        if(++m_Probe.Face >= 6)
        {
            m_Probe.Face          = -1;
            m_Probe.PendingFilter = true;
        }
    }

    // Start of the frame after the last face went to the GPU: prefilter it like a sky bake (one stall)
    void SceneRenderer::FinishProbe()
    {
        if(!m_Probe.PendingFilter)
            return;
        LUMOS_PROFILE_FUNCTION();
        m_Probe.PendingFilter = false;
        Graphics::Renderer::GetGraphicsContext()->WaitIdle();

        auto& quality  = Application::Get().GetQualitySettings();
        auto filtered  = TextureCube::Create(Maths::Max(quality.EnvironmentMapSize, m_Probe.Size), nullptr, true);
        auto irradiance = TextureCube::Create(quality.IrradianceMapSize, nullptr, true);

        auto commandBuffer = CommandBuffer::Create();
        commandBuffer->Init(true);
        commandBuffer->BeginRecording();
        FilterCubeMap(commandBuffer, m_Probe.Cube.get(), filtered, irradiance);
        commandBuffer->EndRecording();
        commandBuffer->Submit();
        Graphics::Renderer::GetGraphicsContext()->WaitIdle();
        delete commandBuffer;

        if(m_Probe.Index < (int)m_Probes.Size())
        {
            m_Probes[m_Probe.Index].Filtered   = SharedPtr<TextureCube>(filtered);
            m_Probes[m_Probe.Index].Irradiance = SharedPtr<TextureCube>(irradiance);
        }
        m_ForwardData.m_EnvironmentMap = nullptr; // rebuilt with the probe's maps below
        LINFO("Environment probe captured at %.1f %.1f %.1f", m_Probe.Position.x, m_Probe.Position.y, m_Probe.Position.z);
    }

    void SceneRenderer::FilterCubeMap(CommandBuffer* commandBuffer, TextureCube* env, TextureCube* filtered, TextureCube* irr)
    {
        // Generate Mips
        {
            commandBuffer->UnBindPipeline();
            commandBuffer->EndCurrentRenderPass();
            env->GenerateMipMaps(commandBuffer);
            auto shader = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("EnvironmentMipFilter")).As<Graphics::Shader>();

            Graphics::DescriptorDesc descriptorDesc {};
            descriptorDesc.layoutIndex = 0;
            descriptorDesc.shader      = shader.get();
            auto descriptorSet         = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
            descriptorSet->SetTexture(0, env);
            descriptorSet->Update(commandBuffer);

            const float deltaRoughness = 1.0f / Maths::Max((float)filtered->GetMipMapLevels() - 1.0f, 1.0f);

            for(uint32_t mip = 0; mip < filtered->GetMipMapLevels(); mip++)
            {
                Graphics::PipelineDesc pipelineDesc {};
                pipelineDesc.shader              = shader;
                pipelineDesc.polygonMode         = Graphics::PolygonMode::FILL;
                pipelineDesc.cullMode            = Graphics::CullMode::BACK;
                pipelineDesc.transparencyEnabled = false;
                pipelineDesc.cubeMapTarget       = filtered;
                pipelineDesc.DebugName           = "Mip Generation";

                for(uint32_t i = 0; i < 6; i++)
                {
                    pipelineDesc.cubeMapIndex = i;
                    pipelineDesc.mipIndex     = mip;
                    auto pipeline             = Graphics::Pipeline::Get(pipelineDesc);
                    commandBuffer->BindPipeline(pipeline, i);

                    auto& pushConstants = shader->GetPushConstants();
                    if(!pushConstants.Empty())
                    {
                        float roughness = mip * deltaRoughness;
                        roughness       = Maths::Max(roughness, 0.05f);

                        auto& pushConstant = shader->GetPushConstants()[0];
                        pushConstant.SetValue("Roughness", (void*)&roughness);
                        pushConstant.SetValue("cubeFaceIndex", (void*)&i);
                        shader->BindPushConstants(commandBuffer, pipeline.get());
                    }

                    auto set = descriptorSet.get();
                    Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
                    Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);
                }
            }
        }

        {
            commandBuffer->UnBindPipeline();
            commandBuffer->EndCurrentRenderPass();

            auto shader = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("EnvironmentIrradiance")).As<Graphics::Shader>();

            Graphics::DescriptorDesc descriptorDesc {};
            descriptorDesc.layoutIndex = 0;
            descriptorDesc.shader      = shader;
            auto descriptorSet         = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));

            descriptorSet->SetTexture(0, filtered);

            uint32_t samples = Application::Get().GetQualitySettings().EnvironmentSamples;
            descriptorSet->SetUniformBufferData(1, (void*)&samples);
            descriptorSet->Update(commandBuffer);

            Graphics::PipelineDesc pipelineDesc {};
            pipelineDesc.shader              = shader;
            pipelineDesc.polygonMode         = Graphics::PolygonMode::FILL;
            pipelineDesc.cullMode            = Graphics::CullMode::BACK;
            pipelineDesc.transparencyEnabled = false;
            pipelineDesc.cubeMapTarget       = irr;

            for(uint32_t i = 0; i < 6; i++)
            {
                pipelineDesc.cubeMapIndex = i;

                auto pipeline = Graphics::Pipeline::Get(pipelineDesc);
                commandBuffer->BindPipeline(pipeline, i);

                auto& pushConstants = shader->GetPushConstants();
                if(!pushConstants.Empty())
                {
                    auto& pushConstant = shader->GetPushConstants()[0];
                    pushConstant.SetValue("cubeFaceIndex", (void*)&i);
                    shader->BindPushConstants(commandBuffer, pipeline.get());
                }

                auto set = descriptorSet.get();
                Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
                Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);
            }
        }

        commandBuffer->UnBindPipeline();
        commandBuffer->EndCurrentRenderPass();
        irr->GenerateMipMaps(commandBuffer);
    }

    void SceneRenderer::CreateCubeMap(const std::string& filePath, const Vec4& params, SharedPtr<TextureCube>& outEnv, SharedPtr<TextureCube>& outIrr)
    {
        // Create shader and pipeline
        // Create Empty Cube Map
        auto environmentMap         = TextureCube::Create(Application::Get().GetQualitySettings().EnvironmentMapSize, nullptr, true);
        auto environmentMapFiltered = TextureCube::Create(Application::Get().GetQualitySettings().EnvironmentMapSize, nullptr, true);
        auto irradianceMap          = TextureCube::Create(Application::Get().GetQualitySettings().IrradianceMapSize, nullptr, true);

        auto commandBuffer = CommandBuffer::Create();
        commandBuffer->Init(true);
        commandBuffer->BeginRecording();
        SharedPtr<Graphics::DescriptorSet> bakeSet;
        SharedPtr<Texture> hdri = nullptr;
        u32 hdriWidth           = 0;
        {
            auto shader = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("CreateEnvironmentMap")).As<Graphics::Shader>();
            if(!shader)
                return;
            Graphics::DescriptorDesc descriptorDesc {};
            descriptorDesc.layoutIndex = 0;
            descriptorDesc.shader      = shader.get();
            auto descriptorSet         = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
            bakeSet                    = descriptorSet;

            if(!filePath.empty())
            {
                // Load hdri image
                TextureDesc textureParams;
                textureParams.srgb = false;
                hdri               = Texture2D::CreateFromFile("Environment", filePath, textureParams);
                descriptorSet->SetTexture(0, hdri);
                if(hdri)
                    hdriWidth = hdri->GetWidth();
            }
            else
            {
                // no panorama: the slot carries the clouds' noise for the procedural sky
                descriptorSet->SetTexture(0, GetSkyNoiseTexture());
            }

            // the whole block every time: the shader's buffer is this size whatever the mode
            // never from the scene: this runs in the middle of loading one
            SkyBakeData bake = s_SkyLatestValid ? s_SkyLatest : BuildSkyBake(nullptr, params);
            bake.Parameters  = params;
            bake.SunColour.w = (float)Engine::GetTimeStep().GetElapsedSeconds();
            if(params.w > 0.5f)
            {
                s_SkyBaked           = bake;
                s_SkyFramesSinceBake = 0;
            }
            descriptorSet->SetUniformBufferData(1, (void*)&bake);
            descriptorSet->Update(commandBuffer);

            Graphics::PipelineDesc pipelineDesc {};
            pipelineDesc.shader              = shader;
            pipelineDesc.polygonMode         = Graphics::PolygonMode::FILL;
            pipelineDesc.cullMode            = Graphics::CullMode::BACK;
            pipelineDesc.transparencyEnabled = false;
            pipelineDesc.cubeMapTarget       = environmentMap;
            pipelineDesc.DebugName           = "Create Cubemap";

            for(uint32_t i = 0; i < 6; i++)
            {
                pipelineDesc.cubeMapIndex = i;

                auto pipeline = Graphics::Pipeline::Get(pipelineDesc);
                commandBuffer->BindPipeline(pipeline, i);

                auto& pushConstants = shader->GetPushConstants();
                if(!pushConstants.Empty())
                {
                    auto& pushConstant = shader->GetPushConstants()[0];
                    pushConstant.SetValue("cubeFaceIndex", (void*)&i);
                    shader->BindPushConstants(commandBuffer, pipeline.get());
                }

                auto set = descriptorSet.get();
                Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
                Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);
            }
        }

        // the panorama again, at its own detail, for the skybox only (one mip, no filtering)
        TextureCube* skyboxCube = nullptr;
        if(hdriWidth > 0)
        {
            const u32 lightingSize = Application::Get().GetQualitySettings().EnvironmentMapSize;
            u32 skySize            = 64;
            while(skySize < hdriWidth / 4 && skySize < 1024)
                skySize *= 2;
            if(skySize > lightingSize)
            {
                skyboxCube = TextureCube::Create(skySize, nullptr, true);
                if(auto shader = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("CreateEnvironmentMap")).As<Graphics::Shader>())
                {
                    Graphics::PipelineDesc skyDesc {};
                    skyDesc.shader        = shader;
                    skyDesc.polygonMode   = Graphics::PolygonMode::FILL;
                    skyDesc.cullMode      = Graphics::CullMode::BACK;
                    skyDesc.cubeMapTarget = skyboxCube;
                    skyDesc.DebugName     = "Create Skybox Cubemap";
                    for(uint32_t i = 0; i < 6; i++)
                    {
                        skyDesc.cubeMapIndex = i;
                        auto pipeline        = Graphics::Pipeline::Get(skyDesc);
                        commandBuffer->BindPipeline(pipeline, i);
                        auto& pushConstants = shader->GetPushConstants();
                        if(!pushConstants.Empty())
                        {
                            pushConstants[0].SetValue("cubeFaceIndex", (void*)&i);
                            shader->BindPushConstants(commandBuffer, pipeline.get());
                        }
                        auto set = bakeSet.get();
                        Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
                        Renderer::Draw(commandBuffer, DrawType::TRIANGLE, 3);
                    }
                    commandBuffer->UnBindPipeline();
                    commandBuffer->EndCurrentRenderPass();
                }
            }
        }

        FilterCubeMap(commandBuffer, environmentMap, environmentMapFiltered, irradianceMap);

        commandBuffer->EndRecording();
        commandBuffer->Submit();

        Graphics::Renderer::GetGraphicsContext()->WaitIdle();
        environmentMap->Destroy(false);
        delete environmentMap;
        delete commandBuffer;
        outEnv = SharedPtr<TextureCube>(environmentMapFiltered);
        outIrr = SharedPtr<TextureCube>(irradianceMap);

        // a freed cube's address can come back: drop any older entry under it
        for(SkyboxCube& old : s_SkyboxCubes)
            if(old.Lighting == (Texture*)environmentMapFiltered)
                old = SkyboxCube();
        SkyboxCube& entry = s_SkyboxCubes[s_SkyboxCubeNext++ % 4];
        entry.Lighting    = environmentMapFiltered;
        entry.Sky         = skyboxCube ? SharedPtr<TextureCube>(skyboxCube) : SharedPtr<TextureCube>();
    }

    void SceneRenderer::InitDebugRenderData()
    {
        if(m_DebugRenderDataInitialised || !m_DebugRenderEnabled)
            return;

        for(int i = 0; i < Renderer::GetMainSwapChain()->GetSwapChainBufferCount(); i++)
        {
            if(m_DebugRenderEnabled)
            {
                m_QuadBufferBase.EmplaceBack().PushBack(new VertexData[m_DebugDrawData.m_Renderer2DData.m_Limits.MaxQuads * 4]);

                // m_2DBufferBase.PushBack(new VertexData[m_Renderer2DData.m_Limits.MaxQuads  * 4]);
                m_LineBufferBase.EmplaceBack().PushBack(new LineVertexData[m_DebugDrawData.m_Renderer2DData.m_Limits.MaxQuads * 4]);
                m_PointBufferBase.EmplaceBack().PushBack(new PointVertexData[m_DebugDrawData.m_Renderer2DData.m_Limits.MaxQuads * 4]);
                DebugTextVertexBufferBase.PushBack(new TextVertexData[m_DebugTextRendererData.m_Limits.MaxQuads * 4]);
            }
        }
        // Points
        m_DebugDrawData.m_PointShader           = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("Batch2DPoint")).As<Graphics::Shader>();
        Graphics::DescriptorDesc descriptorDesc = {};
        descriptorDesc.layoutIndex              = 0;
        descriptorDesc.shader                   = m_DebugDrawData.m_PointShader.get();

        m_DebugDrawData.m_PointDescriptorSet.Resize(1);
        m_DebugDrawData.m_PointDescriptorSet[0] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));

        uint32_t* indices = new uint32_t[MaxPointIndices];

        int32_t offset = 0;
        for(int32_t i = 0; i < MaxPointIndices; i += 6)
        {
            indices[i]     = offset + 0;
            indices[i + 1] = offset + 1;
            indices[i + 2] = offset + 2;

            indices[i + 3] = offset + 2;
            indices[i + 4] = offset + 3;
            indices[i + 5] = offset + 0;

            offset += 4;
        }

        m_DebugDrawData.m_PointIndexBuffer = IndexBuffer::Create(indices, MaxPointIndices);
        delete[] indices;

        // Lines
        m_DebugDrawData.m_LineShader = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("Batch2DLine")).As<Graphics::Shader>();
        descriptorDesc.layoutIndex   = 0;
        descriptorDesc.shader        = m_DebugDrawData.m_LineShader.get();
        m_DebugDrawData.m_LineDescriptorSet.Resize(1);
        m_DebugDrawData.m_LineDescriptorSet[0] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));

        indices = new uint32_t[MaxLineIndices];

        for(int32_t i = 0; i < MaxLineIndices; i++)
        {
            indices[i] = i;
        }

        m_DebugDrawData.m_LineIndexBuffer = IndexBuffer::Create(indices, MaxLineIndices);
        delete[] indices;

        // Debug quads
        m_DebugDrawData.m_Renderer2DData.m_IndexCount           = 0;
        m_DebugDrawData.m_Renderer2DData.m_Buffer               = nullptr;
        m_DebugDrawData.m_Renderer2DData.m_RenderToDepthTexture = true;
        m_DebugDrawData.m_Renderer2DData.m_TriangleIndicies     = false;
        m_DebugDrawData.m_Renderer2DData.m_Limits.SetMaxQuads(10000);
        m_DebugDrawData.m_Renderer2DData.m_Shader = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("Batch2D")).As<Graphics::Shader>();

        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader      = m_DebugDrawData.m_Renderer2DData.m_Shader.get();
        m_DebugDrawData.m_Renderer2DData.m_DescriptorSet.Resize(1);

        m_DebugDrawData.m_Renderer2DData.m_DescriptorSet[0].Resize(2);
        m_DebugDrawData.m_Renderer2DData.m_DescriptorSet[0][0] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
        descriptorDesc.layoutIndex                             = 1;
        m_DebugDrawData.m_Renderer2DData.m_DescriptorSet[0][1] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));

        m_DebugDrawData.m_Renderer2DData.m_VertexBuffers.Resize(3);

        indices = new uint32_t[m_DebugDrawData.m_Renderer2DData.m_Limits.IndiciesSize];

        {
            for(uint32_t i = 0; i < m_DebugDrawData.m_Renderer2DData.m_Limits.IndiciesSize; i++)
            {
                indices[i] = i;
            }
        }
        m_DebugDrawData.m_Renderer2DData.m_IndexBuffer = IndexBuffer::Create(indices, m_Renderer2DData.m_Limits.IndiciesSize);

        delete[] indices;

        // Setup debug text pass
        m_DebugTextRendererData.m_IndexCount = 0;
        // m_TextBuffer                              = nullptr;
        m_DebugTextRendererData.m_RenderToDepthTexture = true;
        m_DebugTextRendererData.m_TriangleIndicies     = false;
        m_DebugTextRendererData.m_Limits.SetMaxQuads(10000);
        m_DebugTextRendererData.m_Limits.MaxTextures = 16; // Renderer::GetCapabilities().MaxTextureUnits;

        DebugTextVertexBufferPtr = DebugTextVertexBufferBase[0];

        m_DebugTextRendererData.m_Shader = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("Text")).As<Graphics::Shader>();

        m_DebugTextRendererData.m_TransformationStack.EmplaceBack(Mat4(1.0f));
        m_DebugTextRendererData.m_TransformationBack = &m_DebugTextRendererData.m_TransformationStack.Back();

        descriptorDesc.layoutIndex = 0;
        descriptorDesc.shader      = m_Renderer2DData.m_Shader.get();
        m_DebugTextRendererData.m_DescriptorSet.Resize(m_DebugTextRendererData.m_Limits.MaxBatchDrawCalls);
        m_DebugTextRendererData.m_PreviousFrameTextureCount.Resize(m_DebugTextRendererData.m_Limits.MaxBatchDrawCalls);

        for(uint32_t i = 0; i < m_DebugTextRendererData.m_Limits.MaxBatchDrawCalls; i++)
        {
            m_DebugTextRendererData.m_PreviousFrameTextureCount[i] = 0;
            m_DebugTextRendererData.m_DescriptorSet[i].Resize(2);
            // if (i == 0)
            {
                descriptorDesc.layoutIndex                    = 0;
                m_DebugTextRendererData.m_DescriptorSet[i][0] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
            }
            descriptorDesc.layoutIndex                    = 1;
            m_DebugTextRendererData.m_DescriptorSet[i][1] = nullptr; // SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
        }

        m_DebugTextRendererData.m_VertexBuffers.Resize(Renderer::GetMainSwapChain()->GetSwapChainBufferCount());
        indices = new uint32_t[m_DebugTextRendererData.m_Limits.IndiciesSize];

        if(m_DebugTextRendererData.m_TriangleIndicies)
        {
            for(uint32_t i = 0; i < m_DebugTextRendererData.m_Limits.IndiciesSize; i++)
            {
                indices[i] = i;
            }
        }
        else
        {
            uint32_t offset = 0;
            for(uint32_t i = 0; i < m_DebugTextRendererData.m_Limits.IndiciesSize; i += 6)
            {
                indices[i]     = offset + 0;
                indices[i + 1] = offset + 1;
                indices[i + 2] = offset + 2;

                indices[i + 3] = offset + 2;
                indices[i + 4] = offset + 3;
                indices[i + 5] = offset + 0;

                offset += 4;
            }
        }
        m_DebugTextRendererData.m_IndexBuffer = IndexBuffer::Create(indices, m_DebugTextRendererData.m_Limits.IndiciesSize);

        delete[] indices;

        m_DebugRenderDataInitialised = true;
    }

    void SceneRenderer::Init2DRenderData()
    {
        m_DebugRenderDataInitialised = true;
    }

    float SceneRenderer::SubmitParticleTexture(Texture* texture)
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        float result = 0.0f;
        bool found   = false;

        for(uint32_t i = 0; i < m_ParticleData.m_TextureCount; i++)
        {
            if(m_ParticleData.m_Textures[i] == texture)
            {
                result = static_cast<float>(i + 1);
                found  = true;
                break;
            }
        }

        if(!found)
        {
            if(m_ParticleData.m_TextureCount >= m_ParticleData.m_Limits.MaxTextures)
            {
                ParticleFlush();
                ParticleBeginBatch();
            }
            m_ParticleData.m_Textures[m_ParticleData.m_TextureCount] = texture;
            m_ParticleData.m_TextureCount++;
            result = static_cast<float>(m_ParticleData.m_TextureCount);
        }
        return result;
    }

    bool compareParticles(const Particle& a, const Particle& b, const Vec3& cameraPosition)
    {
        // Calculate squared distance from camera to particle
        float distanceSqA = Maths::Length2(a.Position - cameraPosition);
        float distanceSqB = Maths::Length2(b.Position - cameraPosition);

        // Sort in descending order based on squared distance
        return distanceSqA > distanceSqB;
    }

    void SceneRenderer::PointCloudPass()
    {
        LUMOS_PROFILE_FUNCTION();

        if(!m_Camera || !m_CameraTransform)
            return;

        auto& clouds = Graphics::GetActivePointClouds();
        if(clouds.Empty())
            return;

        // Lazy init: shader, quad buffers, descriptor sets.
        if(!m_PointCloudInitialised)
        {
            m_PointCloudShader = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("PointCloud")).As<Graphics::Shader>();
            if(!m_PointCloudShader)
                return;

            struct QuadV
            {
                Vec2 corner;
                Vec2 uv;
            };
            QuadV verts[4] = {
                { Vec2(-0.5f, -0.5f), Vec2(0.0f, 0.0f) },
                { Vec2(0.5f, -0.5f), Vec2(1.0f, 0.0f) },
                { Vec2(0.5f, 0.5f), Vec2(1.0f, 1.0f) },
                { Vec2(-0.5f, 0.5f), Vec2(0.0f, 1.0f) },
            };
            uint32_t indices[6] = { 0, 1, 2, 2, 3, 0 };

            m_PointCloudQuadVB = Graphics::VertexBuffer::Create(sizeof(verts), verts, BufferUsage::STATIC);
            m_PointCloudQuadIB = Graphics::IndexBuffer::Create(indices, 6);

            Graphics::DescriptorDesc camDesc {};
            camDesc.layoutIndex          = 0;
            camDesc.shader               = m_PointCloudShader.get();
            m_PointCloudCameraDescriptor = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(camDesc));

            Graphics::DescriptorDesc ptDesc {};
            ptDesc.layoutIndex          = 1;
            ptDesc.shader               = m_PointCloudShader.get();
            m_PointCloudPointDescriptor = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(ptDesc));

            m_PointCloudInitialised = true;
        }

        LUMOS_PROFILE_GPU("PointCloud Pass");

        auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();

        struct PointCloudUBO
        {
            Mat4 projView;
            Vec4 camPos;       // xyz world pos, w = global sky-fade brightness mult
            Vec4 screenParams; // x=width px, y=height px, z=tan(fov/2), w=min size px
            Vec4 streak;       // xyz world travel dir, w amount 0..1 (warp streaks)
            Vec4 look;         // x spike strength, y spike length (sigmas)
        } ubo;
        ubo.projView = m_Camera->GetGPUProjectionMatrix() * Mat4::Inverse(m_CameraTransform->GetWorldMatrix());
        ubo.camPos   = Vec4(m_CameraTransform->GetWorldPosition(), GetPointCloudSkyFade());
        {
            Vec4 look       = GetPointCloudLook();
            float w         = (float)m_MainTexture->GetWidth();
            float h         = (float)m_MainTexture->GetHeight();
            float tanHalf   = Maths::Tan(Maths::ToRadians(m_Camera->GetFOV() * 0.5f));
            // w = the Gaussian sigma floor in pixels. Stars below it are drawn
            // at the floor with their peak scaled down to keep the same energy:
            // a sigma much under a pixel is what twinkles as the camera turns
            // (24% peak swing at 1px, 12% at 1.4 - measured over sub-pixel
            // offsets). Raise it for a steadier but softer field.
            ubo.screenParams = Vec4(w, h, tanHalf, look.z > 0.0f ? look.z : 1.35f);
            ubo.look         = look;
            // w = hidden point index + 1 (0 = none), so a zero from an older UBO
            // layout hides nothing.
            ubo.look.w = (float)(GetPointCloudHiddenIndex() + 1);
        }
        ubo.streak = GetPointCloudStreak();
        m_PointCloudCameraDescriptor->SetUniformBufferData(0, &ubo);
        m_PointCloudCameraDescriptor->Update();

        Graphics::PipelineDesc pipelineDesc;
        pipelineDesc.shader              = m_PointCloudShader;
        pipelineDesc.polygonMode         = Graphics::PolygonMode::FILL;
        pipelineDesc.cullMode            = Graphics::CullMode::NONE;
        pipelineDesc.transparencyEnabled = true;
        pipelineDesc.blendMode           = BlendMode::SrcAlphaOne; // additive glow
        pipelineDesc.clearTargets        = false;
        pipelineDesc.depthTarget         = reinterpret_cast<Texture*>(m_ForwardData.m_DepthTexture);
        pipelineDesc.DepthTest           = true;
        pipelineDesc.DepthWrite          = false;
        pipelineDesc.colourTargets[0]    = m_MainTexture;
        if(m_MainTextureSamples > 1)
            pipelineDesc.resolveTexture = m_ResolveTexture;
        pipelineDesc.samples   = m_MainTextureSamples;
        pipelineDesc.DebugName = "PointCloud";

        auto pipeline = Graphics::Pipeline::Get(pipelineDesc);
        commandBuffer->BindPipeline(pipeline);

        m_PointCloudQuadVB->Bind(commandBuffer, pipeline.get());
        m_PointCloudQuadIB->Bind(commandBuffer);

        for(auto& cloud : clouds)
        {
            if(!cloud || cloud->GetCount() == 0 || !cloud->GetBuffer())
                continue;

            m_PointCloudPointDescriptor->SetStorageBuffer(0, cloud->GetBuffer());
            m_PointCloudPointDescriptor->Update();

            Graphics::DescriptorSet* sets[2] = { m_PointCloudCameraDescriptor.get(), m_PointCloudPointDescriptor.get() };
            Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, sets, 2);
            Renderer::DrawIndexedInstanced(commandBuffer, DrawType::TRIANGLE, 6, cloud->GetCount(), 0);

            m_Stats.NumRenderedObjects += cloud->GetCount();
        }

        m_PointCloudQuadVB->Unbind();
        m_PointCloudQuadIB->Unbind();
        commandBuffer->UnBindPipeline();
        commandBuffer->EndCurrentRenderPass();
    }

    void SceneRenderer::LineCloudPass()
    {
        LUMOS_PROFILE_FUNCTION();

        if(!m_Camera || !m_CameraTransform)
        {
            Graphics::ResetImmediateLineClouds(); // drop this frame's submissions or they accumulate
            return;
        }

        auto& clouds = Graphics::GetActiveLineClouds();

        TDArray<Graphics::LineCloud*> drawList;
        for(auto& cloud : clouds)
            if(cloud && cloud->GetCount() > 0)
                drawList.PushBack(cloud.get());
        for(int i = 0; i < 2; i++)
        {
            Graphics::LineCloud* imm = Graphics::GetImmediateLineCloud(i == 1);
            imm->Finish();
            if(imm->GetCount() > 0)
                drawList.PushBack(imm);
        }

        if(drawList.Empty())
            return;

        if(!m_LineCloudInitialised)
        {
            m_LineCloudShader = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("LineCloud")).As<Graphics::Shader>();
            if(!m_LineCloudShader)
            {
                Graphics::ResetImmediateLineClouds();
                return;
            }

            // Corner buffer: x = end (0=A, 1=B), y = side (-1..+1).
            struct CornerV
            {
                Vec2 c;
            };
            CornerV verts[4] = {
                { Vec2(0.0f, -1.0f) },
                { Vec2(1.0f, -1.0f) },
                { Vec2(1.0f, 1.0f) },
                { Vec2(0.0f, 1.0f) },
            };
            uint32_t indices[6] = { 0, 1, 2, 2, 3, 0 };

            m_LineCloudQuadVB = Graphics::VertexBuffer::Create(sizeof(verts), verts, BufferUsage::STATIC);
            m_LineCloudQuadIB = Graphics::IndexBuffer::Create(indices, 6);

            m_LineCloudInitialised = true;
        }

        // Grow per-cloud descriptor sets to cover this frame's cloud count.
        while(m_LineCloudCameraDescriptors.Size() < drawList.Size())
        {
            Graphics::DescriptorDesc camDesc {};
            camDesc.layoutIndex = 0;
            camDesc.shader      = m_LineCloudShader.get();
            m_LineCloudCameraDescriptors.PushBack(SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(camDesc)));

            Graphics::DescriptorDesc lineDesc {};
            lineDesc.layoutIndex = 1;
            lineDesc.shader      = m_LineCloudShader.get();
            m_LineCloudLineDescriptors.PushBack(SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(lineDesc)));
        }

        LUMOS_PROFILE_GPU("LineCloud Pass");

        auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();

        struct LineCloudUBO
        {
            Mat4 projView;
            Vec4 camPos;
            Vec4 screenParams; // x=width px, y=height px, z=tan(fov/2), w=min width px
        } ubo;

        float scrW       = (float)m_MainTexture->GetWidth();
        float scrH       = (float)m_MainTexture->GetHeight();
        float scrTanHalf = Maths::Tan(Maths::ToRadians(m_Camera->GetFOV() * 0.5f));
        ubo.screenParams = Vec4(scrW, scrH, scrTanHalf, 1.5f); // 1.5px minimum line width

        Mat4 worldProjView = m_Camera->GetGPUProjectionMatrix() * Mat4::Inverse(m_CameraTransform->GetWorldMatrix());
        Vec3 worldEye      = m_CameraTransform->GetWorldPosition();
        bool haveLocal     = Graphics::SystemFrameEnabled();
        Vec3 localEye      = Graphics::SystemFrameCamLocal();
        Mat4 localProjView = worldProjView;
        if(haveLocal)
        {
            Mat4 localWorld = m_CameraTransform->GetWorldMatrix();
            localWorld.SetTranslation(localEye);
            Mat4 localProj  = ReversedPerspective(1e-4f, 1e5f, m_Camera->GetAspectRatio(), m_Camera->GetFOV());
            localProjView   = localProj * localWorld.Inverse();
        }

        Graphics::PipelineDesc pipelineDesc;
        pipelineDesc.shader              = m_LineCloudShader;
        pipelineDesc.polygonMode         = Graphics::PolygonMode::FILL;
        pipelineDesc.cullMode            = Graphics::CullMode::NONE;
        pipelineDesc.transparencyEnabled = true;
        // Premultiplied over-blend. This pass used to be additive, which made
        // every overlap accumulate: a path is dozens of short quads whose ends
        // meet at each join, so the joins beaded brighter than the line itself.
        // The glow additive was there for now comes from the shader's HDR core
        // boost feeding bloom. The shader premultiplies only when told to (see
        // camPos.w below), so the .spv stays correct on an older binary.
        pipelineDesc.blendMode           = BlendMode::OneMinusSrcAlpha;
        pipelineDesc.clearTargets        = false;
        pipelineDesc.depthTarget         = reinterpret_cast<Texture*>(m_ForwardData.m_DepthTexture);
        pipelineDesc.DepthTest           = true;
        pipelineDesc.DepthWrite          = false;
        pipelineDesc.colourTargets[0]    = m_MainTexture;
        if(m_MainTextureSamples > 1)
            pipelineDesc.resolveTexture = m_ResolveTexture;
        pipelineDesc.samples   = m_MainTextureSamples;
        pipelineDesc.DebugName = "LineCloud";

        auto pipeline = Graphics::Pipeline::Get(pipelineDesc);
        commandBuffer->BindPipeline(pipeline);

        m_LineCloudQuadVB->Bind(commandBuffer, pipeline.get());
        m_LineCloudQuadIB->Bind(commandBuffer);

        for(uint32_t ci = 0; ci < (uint32_t)drawList.Size(); ci++)
        {
            Graphics::LineCloud* cloud = drawList[ci];
            if(!cloud->GetBuffer())
                continue;

            bool useLocal = haveLocal && cloud->IsLocal();
            ubo.projView  = useLocal ? localProjView : worldProjView;
            // w = 1 tells the shader this pass expects PREMULTIPLIED colour.
            ubo.camPos    = Vec4(useLocal ? localEye : worldEye, 1.0f);

            auto camDesc  = m_LineCloudCameraDescriptors[ci];
            auto lineDesc = m_LineCloudLineDescriptors[ci];
            camDesc->SetUniformBufferData(0, &ubo);
            camDesc->Update();
            lineDesc->SetStorageBuffer(0, cloud->GetBuffer());
            lineDesc->Update();

            Graphics::DescriptorSet* sets[2] = { camDesc.get(), lineDesc.get() };
            Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, sets, 2);
            Renderer::DrawIndexedInstanced(commandBuffer, DrawType::TRIANGLE, 6, cloud->GetCount(), 0);
        }

        m_LineCloudQuadVB->Unbind();
        m_LineCloudQuadIB->Unbind();
        commandBuffer->UnBindPipeline();
        commandBuffer->EndCurrentRenderPass();

        Graphics::ResetImmediateLineClouds();
    }

    void SceneRenderer::PlanetPass()
    {
        LUMOS_PROFILE_FUNCTION();

        if(!m_Camera || !m_CameraTransform)
            return;

        auto& planets = Graphics::GetPlanetInstances();
        if(planets.Empty())
            return;

        if(!m_PlanetInitialised)
        {
            m_PlanetShader = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("Planet")).As<Graphics::Shader>();
            if(!m_PlanetShader)
            {
                Graphics::ClearPlanets();
                return;
            }

            struct QuadV
            {
                Vec2 corner;
            };
            QuadV verts[4] = {
                { Vec2(-1.0f, -1.0f) }, { Vec2(1.0f, -1.0f) },
                { Vec2(1.0f, 1.0f) }, { Vec2(-1.0f, 1.0f) },
            };
            uint32_t indices[6] = { 0, 1, 2, 2, 3, 0 };
            m_PlanetQuadVB = Graphics::VertexBuffer::Create(sizeof(verts), verts, BufferUsage::STATIC);
            m_PlanetQuadIB = Graphics::IndexBuffer::Create(indices, 6);

            Graphics::DescriptorDesc camDesc {};
            camDesc.layoutIndex      = 0;
            camDesc.shader           = m_PlanetShader.get();
            m_PlanetCameraDescriptor = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(camDesc));

            Graphics::DescriptorDesc instDesc {};
            instDesc.layoutIndex       = 1;
            instDesc.shader            = m_PlanetShader.get();
            m_PlanetInstanceDescriptor = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(instDesc));

            m_PlanetInitialised = true;
        }

        Graphics::StorageBuffer* buffer = Graphics::PlanetUploadBuffer(Renderer::GetMainSwapChain()->GetCurrentBufferIndex());
        if(!buffer)
            return;

        LUMOS_PROFILE_GPU("Planet Pass");
        auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();

        struct PlanetUBO
        {
            Mat4 projView;
            Vec4 camPos;   // xyz cam pos (world or local frame), w = time
            Vec4 camRight;
            Vec4 camUp;
            Vec4 screenParams; // x=width px, y=height px, z=tan(fov/2), w=min planet px
        } ubo;

        // Local system frame (to-scale AU rendering) vs absolute world.
        Vec3 camEye;
        if(Graphics::SystemFrameEnabled())
        {
            camEye              = Graphics::SystemFrameCamLocal();
            Mat4 localWorld     = m_CameraTransform->GetWorldMatrix(); // same rotation
            localWorld.SetTranslation(camEye);                         // eye in local frame
            // Own near/far in AU (the camera's are parsecs for the star field).
            Mat4 localProj      = ReversedPerspective(1e-4f, 1e5f, m_Camera->GetAspectRatio(), m_Camera->GetFOV());
            ubo.projView        = localProj * localWorld.Inverse();
        }
        else
        {
            camEye       = m_CameraTransform->GetWorldPosition();
            ubo.projView = m_Camera->GetGPUProjectionMatrix() * Mat4::Inverse(m_CameraTransform->GetWorldMatrix());
        }
        ubo.camPos   = Vec4(camEye, (float)Engine::GetTimeStep().GetElapsedSeconds());
        ubo.camRight = Vec4(m_CameraTransform->GetRightDirection(), 0.0f);
        ubo.camUp    = Vec4(m_CameraTransform->GetUpDirection(), 0.0f);
        ubo.screenParams = Vec4((float)m_MainTexture->GetWidth(), (float)m_MainTexture->GetHeight(),
                                Maths::Tan(Maths::ToRadians(m_Camera->GetFOV() * 0.5f)), 6.0f);
        m_PlanetCameraDescriptor->SetUniformBufferData(0, &ubo);
        m_PlanetCameraDescriptor->Update();

        m_PlanetInstanceDescriptor->SetStorageBuffer(0, buffer);
        m_PlanetInstanceDescriptor->Update();

        Graphics::PipelineDesc pipelineDesc;
        pipelineDesc.shader      = m_PlanetShader;
        pipelineDesc.polygonMode = Graphics::PolygonMode::FILL;
        pipelineDesc.cullMode    = Graphics::CullMode::NONE;
        pipelineDesc.transparencyEnabled = true;
        pipelineDesc.blendMode           = Graphics::BlendMode::SrcAlphaOneMinusSrcAlpha;
        pipelineDesc.clearTargets        = false;
        pipelineDesc.depthTarget         = reinterpret_cast<Texture*>(m_ForwardData.m_DepthTexture);
        pipelineDesc.DepthTest           = true;
        pipelineDesc.DepthWrite          = true;
        pipelineDesc.colourTargets[0]    = m_MainTexture;
        if(m_MainTextureSamples > 1)
            pipelineDesc.resolveTexture = m_ResolveTexture;
        pipelineDesc.samples   = m_MainTextureSamples;
        pipelineDesc.DebugName = "Planet";

        auto pipeline = Graphics::Pipeline::Get(pipelineDesc);
        commandBuffer->BindPipeline(pipeline);

        m_PlanetQuadVB->Bind(commandBuffer, pipeline.get());
        m_PlanetQuadIB->Bind(commandBuffer);

        Graphics::DescriptorSet* sets[2] = { m_PlanetCameraDescriptor.get(), m_PlanetInstanceDescriptor.get() };
        Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, sets, 2);
        Renderer::DrawIndexedInstanced(commandBuffer, DrawType::TRIANGLE, 6, (uint32_t)planets.Size(), 0);
        m_Stats.NumRenderedObjects += (uint32_t)planets.Size();

        m_PlanetQuadVB->Unbind();
        m_PlanetQuadIB->Unbind();
        commandBuffer->UnBindPipeline();
        commandBuffer->EndCurrentRenderPass();

        // Immediate-mode: consumed this frame, cleared so planets don't accumulate.
        Graphics::ClearPlanets();
    }

    void SceneRenderer::ParticlePass()
    {
        LUMOS_PROFILE_FUNCTION();

        if(!m_Camera || !m_CameraTransform)
            return;

        auto emitterGroup = m_CurrentScene->GetRegistry().group<ParticleEmitter>(entt::get<Maths::Transform>);

        if(emitterGroup.empty())
            return;

        LUMOS_PROFILE_GPU("Particle Pass");

        Graphics::PipelineDesc pipelineDesc;
        pipelineDesc.shader              = m_ParticleData.m_Shader;
        pipelineDesc.polygonMode         = Graphics::PolygonMode::FILL;
        pipelineDesc.cullMode            = Graphics::CullMode::BACK;
        pipelineDesc.transparencyEnabled = true;
        pipelineDesc.blendMode           = BlendMode::SrcAlphaOne;
        pipelineDesc.clearTargets        = false;
        pipelineDesc.depthTarget         = reinterpret_cast<Texture*>(m_ForwardData.m_DepthTexture);
        pipelineDesc.DepthTest           = true;
        pipelineDesc.DepthWrite          = false;
        pipelineDesc.colourTargets[0]    = m_MainTexture;
        if(m_MainTextureSamples > 1)
            pipelineDesc.resolveTexture = m_ResolveTexture;
        pipelineDesc.DebugName = "Particle";
        pipelineDesc.samples   = m_MainTextureSamples;
        ParticleBeginBatch();

        auto projView = m_Camera->GetGPUProjectionMatrix() * Mat4::Inverse(m_CameraTransform->GetWorldMatrix());

        auto cameraPos = m_CameraTransform->GetWorldPosition();
        m_ParticleData.m_DescriptorSet[0][0]->SetUniformBufferData(0, &projView);
        m_ParticleData.m_DescriptorSet[0][0]->Update();

        for(auto& emitterEntity : emitterGroup)
        {
            const auto& [emitter, trans] = emitterGroup.get<ParticleEmitter, Maths::Transform>(emitterEntity);
            Particle* particles          = emitter.GetParticles();
            uint32_t particleCount       = emitter.GetParticleCount();

            if(!particleCount)
                continue;

            const Vec3 emitterOffset = emitter.GetLocalSpace() ? trans.GetWorldPosition() : Vec3(0.0f);

            if(emitter.GetBlendType() == ParticleEmitter::BlendType::Additive)
                pipelineDesc.blendMode = BlendMode::SrcAlphaOne;
            else
                pipelineDesc.blendMode = BlendMode::SrcAlphaOneMinusSrcAlpha;

            pipelineDesc.clearTargets            = false;
            pipelineDesc.DepthTest               = emitter.GetDepthWrite();
            pipelineDesc.DepthWrite              = emitter.GetDepthWrite();
            pipelineDesc.DebugName               = "Particle";
            pipelineDesc.depthBiasEnabled        = true;
            pipelineDesc.depthBiasConstantFactor = -1.25f;
            pipelineDesc.depthBiasSlopeFactor    = -1.75f;
            m_ParticleData.m_Pipeline            = Graphics::Pipeline::Get(pipelineDesc);

            if(emitter.GetSortParticles())
            {
                Algorithms::IntroSort(&particles[0], &particles[particleCount], [&](const Particle& a, const Particle& b)
                          { return compareParticles(a, b, m_CameraTransform->GetWorldPosition() - emitterOffset); });
            }

            for(uint32_t i = 0; i < particleCount; i++)
            {
                auto& particle = particles[i];
                if(particle.Life <= 0.0f)
                    continue;

                m_Stats.NumRenderedObjects++;

                if(m_ParticleData.m_IndexCount >= m_ParticleData.m_Limits.IndiciesSize)
                {
                    ParticleFlush();
                    ParticleBeginBatch();
                }
                const Vec3 basePos = particle.Position + emitterOffset;
                Vec3 v1;
                Vec3 v2;
                Vec3 v3;
                Vec3 v4;
                auto alignType = emitter.GetAlignedType();
                if(alignType == ParticleEmitter::Aligned2D)
                {
                    Vec3 rightOffset = Vec3(1.0f, 0.0f, 0.0f) * particle.Size * 0.5f;
                    Vec3 upOffset    = Vec3(0.0f, 1.0f, 0.0f) * particle.Size * 0.5f;

                    v1 = basePos - rightOffset - upOffset;
                    v2 = basePos + rightOffset - upOffset;
                    v3 = basePos + rightOffset + upOffset;
                    v4 = basePos - rightOffset + upOffset;
                }
                else if(alignType == ParticleEmitter::Aligned3D)
                {
                    Vec3 cameraRight = m_CameraTransform->GetRightDirection().Normalised();
                    Vec3 cameraUp    = m_CameraTransform->GetUpDirection().Normalised();

                    Vec3 rightOffset = cameraRight * particle.Size * 0.5f;
                    Vec3 upOffset    = cameraUp * particle.Size * 0.5f;

                    v1 = basePos - rightOffset - upOffset;
                    v2 = basePos + rightOffset - upOffset;
                    v3 = basePos + rightOffset + upOffset;
                    v4 = basePos - rightOffset + upOffset;
                }
                else
                {
                    Vec3 rightOffset = Vec3(particle.Size * 0.5f, 0.0f, 0.0f);
                    Vec3 upOffset    = Vec3(0.0f, particle.Size * 0.5f, 0.0f);

                    v1 = basePos - rightOffset - upOffset;
                    v2 = basePos + rightOffset - upOffset;
                    v3 = basePos + rightOffset + upOffset;
                    v4 = basePos - rightOffset + upOffset;
                }

                const Vec4 colour = particle.Colour;
                bool animated     = emitter.GetIsAnimated();
                std::array<Vec2, 4> uv;
                std::array<Vec4, 4> blendedUVs;

                float blendAmount = -1.0f;

                if(animated)
                {
                    blendedUVs = emitter.GetBlendedAnimatedUVs(1.0f - (particle.Life / emitter.GetParticleLife()), emitter.GetAnimatedTextureRows(), blendAmount);
                }
                else
                {
                    uv = emitter.GetDefaultUVs();
                }

                Texture* texture = emitter.GetTexture();

                if(!texture)
                    texture = Graphics::Material::GetDefaultTexture();

                float textureSlot = 0.0f;
                if(texture)
                    textureSlot = SubmitParticleTexture(texture);

                m_ParticleData.m_Buffer->vertex = v1;
                if(animated)
                {
                    m_ParticleData.m_Buffer->uv = blendedUVs[0];
                }
                else
                {
                    m_ParticleData.m_Buffer->uv.x = uv[0].x;
                    m_ParticleData.m_Buffer->uv.y = uv[0].y;
                }

                m_ParticleData.m_Buffer->tid    = Vec2(textureSlot, blendAmount);
                m_ParticleData.m_Buffer->colour = colour;
                m_ParticleData.m_Buffer++;

                m_ParticleData.m_Buffer->vertex = v2;
                if(animated)
                {
                    m_ParticleData.m_Buffer->uv = blendedUVs[1];
                }
                else
                {
                    m_ParticleData.m_Buffer->uv.x = uv[1].x;
                    m_ParticleData.m_Buffer->uv.y = uv[1].y;
                }
                m_ParticleData.m_Buffer->tid    = Vec2(textureSlot, blendAmount);
                m_ParticleData.m_Buffer->colour = colour;
                m_ParticleData.m_Buffer++;

                m_ParticleData.m_Buffer->vertex = v3;
                if(animated)
                {
                    m_ParticleData.m_Buffer->uv = blendedUVs[2];
                }
                else
                {
                    m_ParticleData.m_Buffer->uv.x = uv[2].x;
                    m_ParticleData.m_Buffer->uv.y = uv[2].y;
                }
                m_ParticleData.m_Buffer->tid    = Vec2(textureSlot, blendAmount);
                m_ParticleData.m_Buffer->colour = colour;
                m_ParticleData.m_Buffer++;

                m_ParticleData.m_Buffer->vertex = v4;
                if(animated)
                {
                    m_ParticleData.m_Buffer->uv = blendedUVs[3];
                }
                else
                {
                    m_ParticleData.m_Buffer->uv.x = uv[3].x;
                    m_ParticleData.m_Buffer->uv.y = uv[3].y;
                }
                m_ParticleData.m_Buffer->tid    = Vec2(textureSlot, blendAmount);
                m_ParticleData.m_Buffer->colour = colour;
                m_ParticleData.m_Buffer++;

                m_ParticleData.m_IndexCount += 6;
            }
        }

        if(m_ParticleData.m_IndexCount == 0)
        {
            return;
        }

        ParticleFlush();
    }

    void SceneRenderer::ParticleBeginBatch()
    {
        uint32_t currentFrame = Renderer::GetMainSwapChain()->GetCurrentBufferIndex();

        m_ParticleData.m_IndexCount   = 0;
        m_ParticleData.m_TextureCount = 0;

        while((int)m_ParticleData.m_VertexBuffers[currentFrame].Size() - 1 < (int)m_ParticleData.m_BatchDrawCallIndex)
        {
            auto& vertexBuffer = m_ParticleData.m_VertexBuffers[currentFrame].EmplaceBack(Graphics::VertexBuffer::Create(BufferUsage::DYNAMIC));
            vertexBuffer->Resize(BATCH_BUFFER_INITIAL);
        }

        m_ParticleData.m_Buffer = m_ParticleBufferBase[currentFrame][0];
    }

    void SceneRenderer::ParticleFlush()
    {
        LUMOS_PROFILE_FUNCTION();
        uint32_t currentFrame = Renderer::GetMainSwapChain()->GetCurrentBufferIndex();
        for(int i = m_ParticleData.m_TextureCount; i < 16; i++)
        {
            m_ParticleData.m_Textures[i] = Material::GetDefaultTexture();
        }

        m_ParticleData.m_TextureCount = 16;

        if(m_ParticleData.m_DescriptorSet[m_ParticleData.m_BatchDrawCallIndex][1] == nullptr || m_ParticleData.m_TextureCount != m_ParticleData.m_PreviousFrameTextureCount[m_ParticleData.m_BatchDrawCallIndex])
        {
            /*
             || m_ParticleData.m_TextureCount != m_ParticleData.m_PreviousFrameTextureCount[m_ParticleData.m_BatchDrawCallIndex])
             When previous frame texture count was less then than the previous frame
             and the texture previously used was deleted, there was a crash - maybe moltenvk only
             May not be needed anymore
            */
            Graphics::DescriptorDesc descriptorDesc {};
            descriptorDesc.layoutIndex                                             = 1;
            descriptorDesc.shader                                                  = m_ParticleData.m_Shader.get();
            m_ParticleData.m_DescriptorSet[m_ParticleData.m_BatchDrawCallIndex][1] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
        }

        if(m_ParticleData.m_TextureCount > 1)
            m_ParticleData.m_DescriptorSet[m_ParticleData.m_BatchDrawCallIndex][1]->SetTexture(0, m_ParticleData.m_Textures, m_ParticleData.m_TextureCount);
        else if(m_ParticleData.m_TextureCount == 0)
            m_ParticleData.m_DescriptorSet[m_ParticleData.m_BatchDrawCallIndex][1]->SetTexture(0, nullptr);
        else
            m_ParticleData.m_DescriptorSet[m_ParticleData.m_BatchDrawCallIndex][1]->SetTexture(0, m_ParticleData.m_Textures[0]);

        m_ParticleData.m_DescriptorSet[m_ParticleData.m_BatchDrawCallIndex][1]->Update();

        m_ParticleData.m_PreviousFrameTextureCount[m_ParticleData.m_BatchDrawCallIndex] = m_ParticleData.m_TextureCount;

        Graphics::CommandBuffer* commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();

        // keep the render pass open between batches: ending it reloads / stores the whole target
        uint32_t dataSize = (uint32_t)((uint8_t*)m_ParticleData.m_Buffer - (uint8_t*)m_ParticleBufferBase[currentFrame][0]);
        UploadBatch(m_ParticleData.m_VertexBuffers[currentFrame][m_ParticleData.m_BatchDrawCallIndex], dataSize, m_ParticleBufferBase[currentFrame][0]);
        commandBuffer->BindPipeline(m_ParticleData.m_Pipeline);
        m_ParticleData.m_VertexBuffers[currentFrame][m_ParticleData.m_BatchDrawCallIndex]->Bind(commandBuffer, m_ParticleData.m_Pipeline.get());

        Arena* frameArena                  = Application::Get().GetFrameArena();
        DescriptorSet** currentDescriptors = PushArrayNoZero(frameArena, DescriptorSet*, 2);

        currentDescriptors[0] = m_ParticleData.m_DescriptorSet[0][0].get();
        currentDescriptors[1] = m_ParticleData.m_DescriptorSet[m_ParticleData.m_BatchDrawCallIndex][1].get();

        m_ParticleData.m_IndexBuffer->SetCount(m_ParticleData.m_IndexCount);
        m_ParticleData.m_IndexBuffer->Bind(commandBuffer);

        Renderer::BindDescriptorSets(m_ParticleData.m_Pipeline.get(), commandBuffer, 0, currentDescriptors, 2);
        Renderer::DrawIndexed(commandBuffer, DrawType::TRIANGLE, m_ParticleData.m_IndexCount);

        m_ParticleData.m_VertexBuffers[currentFrame][m_ParticleData.m_BatchDrawCallIndex]->Unbind();
        m_ParticleData.m_IndexBuffer->Unbind();

        m_ParticleData.m_BatchDrawCallIndex++;
        m_ParticleData.m_TextureCount = 0;
    }
}
