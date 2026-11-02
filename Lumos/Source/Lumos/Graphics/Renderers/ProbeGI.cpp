#include "Precompiled.h"
#include "ProbeGI.h"
#include "Scene/Scene.h"
#include "Scene/Entity.h"
#include "Scene/Component/ModelComponent.h"
#include "Scene/Component/RigidBody3DComponent.h"
#include "Graphics/Model.h"
#include "Graphics/Mesh.h"
#include "Graphics/Material.h"
#include "Graphics/Light.h"
#include "Graphics/RHI/Shader.h"
#include "Graphics/RHI/DescriptorSet.h"
#include "Graphics/RHI/Pipeline.h"
#include "Graphics/RHI/StorageBuffer.h"
#include "Graphics/RHI/Texture.h"
#include "Graphics/RHI/CommandBuffer.h"
#include "Graphics/RHI/Renderer.h"
#include "Graphics/RHI/GPUProfile.h"
#include "Graphics/RHI/SwapChain.h"
#include "Core/Application.h"
#include "Core/Asset/AssetManager.h"
#include "Maths/MathsUtilities.h"
#include "Maths/Random.h"
#include "Maths/Transform.h"

namespace Lumos::Graphics
{
    static constexpr u32 kMaxTriangles = 1u << 20;
    static constexpr u32 kMaxProbes    = 8192;
    static constexpr u32 kProbeVec4s   = 37; // GI_PROBE_VEC4S in GI.glslh
    static constexpr u32 kDepthBase    = 5;  // GI_DEPTH_BASE
    static constexpr u32 kMaxGILights  = 32;
    static constexpr u32 kSettleUpdates = 32; // updates per probe before it stops moving and keeps the full hysteresis
    static constexpr u32 kLeafSize     = 4;
    static constexpr u32 kMaxDepth     = 28; // the trace shader's stack holds 32

    struct GIBVHNode
    {
        float Min[3];
        u32 LeftFirst;
        float Max[3];
        u32 Count;
    };

    struct GIBuildTriangle
    {
        Vec3 A, B, C;
        Vec3 Centroid;
        u32 Material;
    };

    struct GITraceData
    {
        Mat4 RayRotation;
        Vec4 Origin;  // w = share of the old value kept
        Vec4 Spacing;
        i32 Counts[4];
        Vec4 SunDirection;
        Vec4 SunColour;
        u32 Update[4]; // stride, offset, nodes, triangles
        Vec4 Params;   // sky, bounce, ray bias, emissive scale
        Vec4 Scales;   // light scale, max depth, closest a wall may be
        u32 Extra[4];  // light count, move probes, light range mode
    };

    struct GIAlbedoPush
    {
        Vec4 Colour;
        Vec4 Emissive;
        u32 Index;
        float Lod;
        float EmissiveLod;
        float Pad0;
    };

    // Partial sort: order[k] ends up with the k-th smallest key in [lo, hi], smaller ones before it
    template <typename Key>
    static void SelectNth(u32* order, i64 lo, i64 hi, i64 k, Key key)
    {
        while(hi > lo)
        {
            const float pivot = key(order[(lo + hi) / 2]);
            i64 i = lo, j = hi;
            while(i <= j)
            {
                while(key(order[i]) < pivot)
                    i++;
                while(key(order[j]) > pivot)
                    j--;
                if(i <= j)
                {
                    const u32 t = order[i];
                    order[i]    = order[j];
                    order[j]    = t;
                    i++;
                    j--;
                }
            }
            if(k <= j)
                hi = j;
            else if(k >= i)
                lo = i;
            else
                return;
        }
    }

    // a model under a moving body (a player's mesh) moves too, and would keep rebuilding the BVH
    static bool IsStaticModel(Entity entity)
    {
        for(int depth = 0; entity.Valid() && depth < 32; depth++)
        {
            if(RigidBody3DComponent* body = entity.TryGetComponent<RigidBody3DComponent>())
                if(body->GetRigidBody() && !body->GetRigidBody()->GetIsStatic())
                    return false;
            entity = entity.GetParent();
        }
        return true;
    }

    ProbeGI::ProbeGI()
    {
        m_Supported = Renderer::GetCapabilities().SupportCompute;
        if(m_Supported)
        {
            auto assets    = Application::Get().GetAssetManager();
            m_TraceShader  = assets->GetAssetData(Str8Lit("GIProbeTrace")).As<Shader>();
            m_AlbedoShader = assets->GetAssetData(Str8Lit("GIMaterialAlbedo")).As<Shader>();
            m_Supported    = m_TraceShader && m_TraceShader->IsCompiled() && m_AlbedoShader && m_AlbedoShader->IsCompiled();
            if(!m_Supported)
                LWARN("GI shaders not available - probe GI off");
        }

        if(m_Supported)
        {
            DescriptorDesc desc {};
            desc.layoutIndex = 0;
            desc.shader      = m_TraceShader.get();
            m_TraceSet       = SharedPtr<DescriptorSet>(DescriptorSet::Create(desc));
        }

        // one empty probe, so the forward shader's binding is always filled
        const Vec4 empty[4] = {};
        m_ProbeBuffer       = StorageBuffer::Create(sizeof(empty), empty);
    }

    ProbeGI::~ProbeGI()
    {
        delete m_NodeBuffer;
        delete m_TriangleBuffer;
        delete m_MaterialBuffer;
        delete m_ProbeBuffer;
        delete m_LightBuffer;
    }

    void ProbeGI::SetEnabled(bool enabled)
    {
        if(enabled && !m_Enabled)
            m_UpdatesSinceReset = 0;
        m_Enabled = enabled;
    }

    u64 ProbeGI::ComputeSignature(Scene* scene, float spacing) const
    {
        u64 h    = 1469598103934665603ull;
        auto mix = [&](u64 v)
        {
            h ^= v;
            h *= 1099511628211ull;
        };
        auto mixFloat = [&](float v)
        {
            u32 b;
            memcpy(&b, &v, 4);
            mix(b);
        };

        mixFloat(spacing);
        auto& registry = scene->GetRegistry();
        auto group     = registry.group<ModelComponent>(entt::get<Maths::Transform>);
        for(auto handle : group)
        {
            Entity entity(handle, scene);
            if(!entity.Active() || !IsStaticModel(entity))
                continue;
            const auto& [model, trans] = group.get<ModelComponent, Maths::Transform>(handle);
            if(!model.ModelRef)
                continue;

            mix((u64)handle);
            const Mat4& world = trans.GetWorldMatrix();
            for(int i = 0; i < 16; i++)
                mixFloat(world.values[i]);
            for(const auto& mesh : model.ModelRef->GetMeshes())
            {
                mix((u64)(uintptr_t)mesh.get());
                mix((u64)mesh->GetCPUIndices().Size());
            }
            mix((u64)model.InstanceMaterials.size());
        }
        return h == 0 ? 1 : h;
    }

    void ProbeGI::Rebuild(Scene* scene, float spacing)
    {
        LUMOS_PROFILE_FUNCTION();
        ArenaTemp scratch = ScratchBegin(nullptr, 0);

        m_Materials.Clear();
        TDArray<GIBuildTriangle> tris;
        u32 skippedMeshes = 0;

        auto& registry = scene->GetRegistry();
        auto group     = registry.group<ModelComponent>(entt::get<Maths::Transform>);
        for(auto handle : group)
        {
            Entity entity(handle, scene);
            if(!entity.Active() || !IsStaticModel(entity))
                continue;
            const auto& [model, trans] = group.get<ModelComponent, Maths::Transform>(handle);
            if(!model.ModelRef)
                continue;

            const Mat4& world  = trans.GetWorldMatrix();
            const auto& meshes = model.ModelRef->GetMeshes();
            for(u32 meshIdx = 0; meshIdx < (u32)meshes.Size(); meshIdx++)
            {
                Mesh* mesh = meshes[meshIdx].get();
                if(mesh->GetAnimVertexBuffer())
                    continue;
                const auto& positions = mesh->GetCPUPositions();
                const auto& indices   = mesh->GetCPUIndices();
                if(positions.Empty() || indices.Size() < 3)
                {
                    skippedMeshes++;
                    continue;
                }

                auto inst          = model.InstanceMaterials.find(meshIdx);
                Material* material = inst != model.InstanceMaterials.end() ? inst->second.get() : mesh->GetMaterial().get();
                u32 materialIndex  = 0;
                for(; materialIndex < m_Materials.Size(); materialIndex++)
                    if(m_Materials[materialIndex] == material)
                        break;
                if(materialIndex == m_Materials.Size())
                    m_Materials.PushBack(material);

                for(u32 i = 0; i + 2 < indices.Size() && tris.Size() < kMaxTriangles; i += 3)
                {
                    const u32 i0 = indices[i], i1 = indices[i + 1], i2 = indices[i + 2];
                    if(i0 >= positions.Size() || i1 >= positions.Size() || i2 >= positions.Size())
                        continue;
                    GIBuildTriangle t;
                    t.A        = Vec3(world * Vec4(positions[i0], 1.0f));
                    t.B        = Vec3(world * Vec4(positions[i1], 1.0f));
                    t.C        = Vec3(world * Vec4(positions[i2], 1.0f));
                    t.Centroid = (t.A + t.B + t.C) * (1.0f / 3.0f);
                    t.Material = materialIndex;
                    tris.PushBack(t);
                }
            }
        }

        if(skippedMeshes)
            LWARN("GI: %u meshes have no CPU copy of their triangles and are left out", skippedMeshes);
        if(tris.Size() >= kMaxTriangles)
            LWARN("GI: scene capped at %u triangles", kMaxTriangles);

        m_TriangleCount = (u32)tris.Size();
        m_NodeCount     = 0;
        if(tris.Empty())
        {
            m_Active = false;
            ScratchEnd(scratch);
            return;
        }

        // ---- BVH: median split on the widest axis of the centroids ----
        const u32 n = (u32)tris.Size();
        // big scenes run to tens of MB: the heap, not the scratch arena
        TDArray<u32> orderStorage;
        orderStorage.Resize(n);
        u32* order = orderStorage.Data();
        for(u32 i = 0; i < n; i++)
            order[i] = i;

        TDArray<GIBVHNode> nodes;
        nodes.Reserve(n / kLeafSize * 2 + 2);
        nodes.EmplaceBack();

        struct BuildItem
        {
            u32 Node, Start, Count, Depth;
        };
        TDArray<BuildItem> stack;
        stack.PushBack({ 0, 0, n, 0 });

        Vec3 sceneMin(1e30f), sceneMax(-1e30f);
        while(!stack.Empty())
        {
            const BuildItem item = stack.Back();
            stack.PopBack();

            Vec3 bmin(1e30f), bmax(-1e30f), cmin(1e30f), cmax(-1e30f);
            for(u32 i = item.Start; i < item.Start + item.Count; i++)
            {
                const GIBuildTriangle& t = tris[order[i]];
                for(const Vec3* p : { &t.A, &t.B, &t.C })
                {
                    bmin = Vec3(Maths::Min(bmin.x, p->x), Maths::Min(bmin.y, p->y), Maths::Min(bmin.z, p->z));
                    bmax = Vec3(Maths::Max(bmax.x, p->x), Maths::Max(bmax.y, p->y), Maths::Max(bmax.z, p->z));
                }
                cmin = Vec3(Maths::Min(cmin.x, t.Centroid.x), Maths::Min(cmin.y, t.Centroid.y), Maths::Min(cmin.z, t.Centroid.z));
                cmax = Vec3(Maths::Max(cmax.x, t.Centroid.x), Maths::Max(cmax.y, t.Centroid.y), Maths::Max(cmax.z, t.Centroid.z));
            }
            if(item.Node == 0)
            {
                sceneMin = bmin;
                sceneMax = bmax;
            }

            GIBVHNode& node = nodes[item.Node];
            node.Min[0] = bmin.x, node.Min[1] = bmin.y, node.Min[2] = bmin.z;
            node.Max[0] = bmax.x, node.Max[1] = bmax.y, node.Max[2] = bmax.z;

            const Vec3 extent = cmax - cmin;
            int axis          = extent.x > extent.y ? (extent.x > extent.z ? 0 : 2) : (extent.y > extent.z ? 1 : 2);
            const float width = axis == 0 ? extent.x : (axis == 1 ? extent.y : extent.z);
            if(item.Count <= kLeafSize || item.Depth >= kMaxDepth || width < 1e-6f)
            {
                node.LeftFirst = item.Start;
                node.Count     = item.Count;
                continue;
            }

            const u32 half = item.Count / 2;
            SelectNth(order, item.Start, item.Start + item.Count - 1, item.Start + half, [&](u32 t)
                      { const Vec3& c = tris[t].Centroid; return axis == 0 ? c.x : (axis == 1 ? c.y : c.z); });

            const u32 left = (u32)nodes.Size();
            node.LeftFirst = left;
            node.Count     = 0;
            nodes.EmplaceBack(); // invalidates `node`
            nodes.EmplaceBack();
            stack.PushBack({ left, item.Start, half, item.Depth + 1 });
            stack.PushBack({ left + 1, item.Start + half, item.Count - half, item.Depth + 1 });
        }
        m_NodeCount = (u32)nodes.Size();

        // triangles in leaf order: 3 vec4s each, the first w carries the material
        TDArray<Vec4> triStorage;
        triStorage.Resize(n * 3);
        Vec4* triData = triStorage.Data();
        for(u32 i = 0; i < n; i++)
        {
            const GIBuildTriangle& t = tris[order[i]];
            float materialBits;
            memcpy(&materialBits, &t.Material, 4);
            triData[i * 3 + 0] = Vec4(t.A, materialBits);
            triData[i * 3 + 1] = Vec4(t.B, 0.0f);
            triData[i * 3 + 2] = Vec4(t.C, 0.0f);
        }

        auto upload = [](StorageBuffer*& buffer, u32 size, const void* data)
        {
            if(!buffer)
                buffer = StorageBuffer::Create(size, data);
            else
                buffer->Resize(size, data);
        };
        upload(m_NodeBuffer, m_NodeCount * sizeof(GIBVHNode), nodes.Data());
        upload(m_TriangleBuffer, n * 3 * sizeof(Vec4), triData);

        // material colours now; the maps are averaged on the GPU at the next update
        // albedo, emissive per material
        const u32 materialCount = Maths::Max(1u, (u32)m_Materials.Size());
        Vec4* albedo            = PushArrayNoZero(scratch.arena, Vec4, materialCount * 2);
        albedo[0]               = Vec4(0.7f, 0.7f, 0.7f, 1.0f);
        albedo[1]               = Vec4(0.0f);
        m_AlbedoSets.Clear();
        for(u32 i = 0; i < m_Materials.Size(); i++)
        {
            Material* material = m_Materials[i];
            albedo[i * 2]      = Vec4(0.7f, 0.7f, 0.7f, 1.0f);
            albedo[i * 2 + 1]  = Vec4(0.0f);
            if(!material || !material->GetProperties())
            {
                m_AlbedoSets.PushBack(nullptr);
                continue;
            }
            const MaterialProperties* props = material->GetProperties();
            const Vec3 colour(props->albedoColour.x, props->albedoColour.y, props->albedoColour.z);
            const Vec3 glow = Vec3(props->emissiveColour.x, props->emissiveColour.y, props->emissiveColour.z) * props->emissiveColour.w;
            albedo[i * 2]     = Vec4(colour, 1.0f);
            albedo[i * 2 + 1] = Vec4(glow.x * colour.x, glow.y * colour.y, glow.z * colour.z, 0.0f);
            const bool albedoMap   = props->albedoMapFactor >= 0.05f && material->GetTextures().albedo;
            const bool emissiveMap = props->emissiveMapFactor >= 0.05f && material->GetTextures().emissive && Maths::Max(glow.x, Maths::Max(glow.y, glow.z)) > 0.0f;
            if(albedoMap || emissiveMap)
            {
                DescriptorDesc desc {};
                desc.layoutIndex = 0;
                desc.shader      = m_AlbedoShader.get();
                m_AlbedoSets.PushBack(SharedPtr<DescriptorSet>(DescriptorSet::Create(desc)));
            }
            else
                m_AlbedoSets.PushBack(nullptr);
        }
        upload(m_MaterialBuffer, materialCount * 2 * sizeof(Vec4), albedo);
        m_AlbedoPending = !m_Materials.Empty();

        // ---- probe grid over everything, half a cell of margin ----
        const u32 oldCounts[3] = { m_Counts[0], m_Counts[1], m_Counts[2] };
        const Vec3 oldOrigin   = m_Origin;
        const Vec3 oldSpacing  = m_Spacing;
        Vec3 lo   = sceneMin - Vec3(spacing * 0.5f);
        Vec3 hi   = sceneMax + Vec3(spacing * 0.5f);
        Vec3 size = hi - lo;
        float s   = Maths::Max(spacing, 0.05f);
        for(int attempt = 0; attempt < 64; attempt++)
        {
            m_Counts[0] = Maths::Max(2u, (u32)ceilf(size.x / s) + 1);
            m_Counts[1] = Maths::Max(2u, (u32)ceilf(size.y / s) + 1);
            m_Counts[2] = Maths::Max(2u, (u32)ceilf(size.z / s) + 1);
            if(m_Counts[0] * m_Counts[1] * m_Counts[2] <= kMaxProbes)
                break;
            s *= 1.1f;
        }
        m_Origin  = lo;
        m_Spacing = Vec3(size.x / float(m_Counts[0] - 1), size.y / float(m_Counts[1] - 1), size.z / float(m_Counts[2] - 1));

        // same grid: the probes keep what they hold and just carry on (clearing them is a visible flash)
        const bool sameGrid = m_ProbeBuffer->GetSize() > 0 && oldCounts[0] == m_Counts[0] && oldCounts[1] == m_Counts[1] && oldCounts[2] == m_Counts[2]
                              && Maths::Length(oldOrigin - m_Origin) < 1e-3f && Maths::Length(oldSpacing - m_Spacing) < 1e-3f;
        if(!sameGrid)
        {
            // no light, not moved, and every direction open as far as the depth map goes
            const float maxDepth = 2.0f * Maths::Max(m_Spacing.x, Maths::Max(m_Spacing.y, m_Spacing.z));
            const u32 probeBytes = GetProbeCount() * kProbeVec4s * sizeof(Vec4);
            TDArray<Vec4> init;
            init.Resize(GetProbeCount() * kProbeVec4s);
            for(u32 p = 0; p < GetProbeCount(); p++)
                for(u32 i = 0; i < kProbeVec4s; i++)
                    init[p * kProbeVec4s + i] = i < kDepthBase ? Vec4(0.0f) : Vec4(maxDepth, maxDepth * maxDepth, maxDepth, maxDepth * maxDepth);
            m_ProbeBuffer->Resize(probeBytes, init.Data());
            m_UpdatesSinceReset = 0;
        }
        else
        {
            // the scene changed under the probes: settle quickly again, and let them move
            m_UpdatesSinceReset = Maths::Min(m_UpdatesSinceReset, m_LastStride * 4);
        }

        LINFO("GI: %u triangles, %u BVH nodes, %u materials, %ux%ux%u probes (%.2f m)", n, m_NodeCount, (u32)m_Materials.Size(), m_Counts[0], m_Counts[1], m_Counts[2], s);
        ScratchEnd(scratch);
    }

    void ProbeGI::Prepare(Scene* scene, bool enabled, const ProbeGIParams& params)
    {
        if(!enabled || !m_Supported || !scene)
        {
            m_Active = false;
            return;
        }

        // the static set is checked twice a second, and rebuilt once it has stopped changing:
        // an object dragged in the editor would otherwise rebuild every frame
        m_FrameCounter++;
        if(m_Signature == 0)
        {
            m_Signature        = ComputeSignature(scene, params.Spacing);
            m_PendingSignature = m_Signature;
            Rebuild(scene, params.Spacing);
        }
        else if(m_FrameCounter % 30 == 0)
        {
            const u64 now = ComputeSignature(scene, params.Spacing);
            if(now == m_Signature)
                m_StableFrames = 0;
            else if(now != m_PendingSignature)
            {
                m_PendingSignature = now;
                m_StableFrames     = 0;
            }
            else if((m_StableFrames += 30) >= 60)
            {
                m_Signature    = now;
                m_StableFrames = 0;
                Rebuild(scene, params.Spacing);
            }
        }

        m_Active = m_TriangleCount > 0 && m_NodeBuffer && m_TriangleBuffer && m_MaterialBuffer;
        if(!m_Active)
            return;

        // point and spot lights, whether in view or not (the forward pass culls to the frustum)
        GPULight lights[kMaxGILights];
        m_LightCount   = 0;
        auto& registry = scene->GetRegistry();
        auto group     = registry.group<Light>(entt::get<Maths::Transform>);
        for(auto handle : group)
        {
            if(m_LightCount >= kMaxGILights)
                break;
            if(!Entity(handle, scene).Active())
                continue;
            const auto& [light, trans] = group.get<Light, Maths::Transform>(handle);
            if(light.Type == float(LightType::DirectionalLight) || light.Intensity <= 0.0f)
                continue;
            GPULight& l = lights[m_LightCount++];
            l           = light.ToGPU();
            l.Position = Vec4(trans.GetWorldPosition(), 1.0f);
            Vec3 forward = trans.GetWorldOrientation() * Vec3(0.0f, 0.0f, 1.0f);
            forward.Normalise();
            l.Direction = Vec4(forward, 1.0f);
        }
        const u32 lightBytes = Maths::Max(1u, m_LightCount) * sizeof(GPULight);
        if(!m_LightBuffer)
            m_LightBuffer = StorageBuffer::Create(lightBytes, lights);
        else if(m_LightCount > 0 || m_LightBuffer->GetSize() < lightBytes)
            m_LightBuffer->Resize(lightBytes, lights);
    }

    void ProbeGI::Update(CommandBuffer* commandBuffer, Texture* environmentMap, const Vec3& sunDirection, const Vec3& sunIlluminance, bool hasSun, const ProbeGIParams& params)
    {
        if(!m_Active || !environmentMap)
            return;

        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_GPU("GI Probe Update");
        commandBuffer->UnBindPipeline();

        if(m_AlbedoPending)
        {
            m_AlbedoPending = false;
            PipelineDesc desc {};
            desc.shader    = m_AlbedoShader;
            desc.DebugName = "GIMaterialAlbedo";
            auto pipeline  = Pipeline::Get(desc);
            commandBuffer->BindPipeline(pipeline);
            for(u32 i = 0; i < m_Materials.Size() && i < m_AlbedoSets.Size(); i++)
            {
                if(!m_AlbedoSets[i])
                    continue;
                Material* material = m_Materials[i];
                // the maps can be gone by now (unloaded, swapped): the colours uploaded at rebuild stay
                if(!material || !material->GetProperties())
                    continue;
                const MaterialProperties* props = material->GetProperties();
                Texture2D* map                  = props->albedoMapFactor >= 0.05f ? material->GetTextures().albedo.get() : nullptr;
                Texture2D* emissiveMap          = props->emissiveMapFactor >= 0.05f ? material->GetTextures().emissive.get() : nullptr;
                if(!map && !emissiveMap)
                    continue;
                Texture2D* fallback = Material::GetDefaultTexture().get();

                m_AlbedoSets[i]->SetTexture(0, map ? map : fallback);
                m_AlbedoSets[i]->SetStorageBuffer(1, m_MaterialBuffer);
                m_AlbedoSets[i]->SetTexture(2, emissiveMap ? emissiveMap : fallback);
                m_AlbedoSets[i]->Update();
                m_AlbedoSets[i]->TransitionImages(commandBuffer);

                GIAlbedoPush push;
                push.Colour      = Vec4(props->albedoColour.x, props->albedoColour.y, props->albedoColour.z, map ? Maths::Clamp(props->albedoMapFactor, 0.0f, 1.0f) : 0.0f);
                push.Emissive    = Vec4(props->emissiveColour.x * props->emissiveColour.w, props->emissiveColour.y * props->emissiveColour.w, props->emissiveColour.z * props->emissiveColour.w, emissiveMap ? 1.0f : 0.0f);
                push.Index       = i;
                push.Lod         = map ? (float)Maths::Max(0, (int)map->GetMipMapLevels() - 4) : 0.0f;
                push.EmissiveLod = emissiveMap ? (float)Maths::Max(0, (int)emissiveMap->GetMipMapLevels() - 4) : 0.0f;
                push.Pad0        = 0.0f;
                m_AlbedoShader->GetPushConstants()[0].SetData(&push);
                m_AlbedoShader->BindPushConstants(commandBuffer, pipeline.get());

                auto set = m_AlbedoSets[i].get();
                Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
                Renderer::GetRenderer()->Dispatch(commandBuffer, 1, 1, 1);
            }
            commandBuffer->BufferMemoryBarrier(CommandBuffer::PipelineStage::COMPUTE_SHADER, CommandBuffer::PipelineStage::COMPUTE_SHADER);
        }

        const u32 stride = Maths::Clamp(params.UpdateStride, 1u, 64u);
        const u32 offset = m_FrameCounter % stride;
        const u32 count  = GetProbeCount();
        if(offset >= count)
            return;
        const u32 groups = (count - offset + stride - 1) / stride;

        // a running mean over the first updates (each new one weighs 1/n), then the set hysteresis:
        // the probes are usable within a second of a reset instead of fading in for ten
        m_LastStride        = stride;
        const u32 perProbe  = m_UpdatesSinceReset / stride;
        const float keep    = Maths::Min(Maths::Clamp(params.Hysteresis, 0.0f, 0.999f), 1.0f - 1.0f / float(perProbe + 1));
        const float minSpacing = Maths::Min(m_Spacing.x, Maths::Min(m_Spacing.y, m_Spacing.z));
        const float maxSpacing = Maths::Max(m_Spacing.x, Maths::Max(m_Spacing.y, m_Spacing.z));

        GITraceData data;
        // a new turn of the ray set every update, so over time each probe looks everywhere
        data.RayRotation  = Quat::EulerAnglesToQuaternion(Random32::Rand(0.0f, 360.0f), Random32::Rand(0.0f, 360.0f), Random32::Rand(0.0f, 360.0f)).ToMatrix4();
        data.Origin       = Vec4(m_Origin, keep);
        data.Spacing      = Vec4(m_Spacing, 0.0f);
        data.Counts[0]    = (i32)m_Counts[0];
        data.Counts[1]    = (i32)m_Counts[1];
        data.Counts[2]    = (i32)m_Counts[2];
        data.Counts[3]    = (i32)count;
        data.SunDirection = Vec4(sunDirection, hasSun ? 1.0f : 0.0f);
        data.SunColour    = Vec4(sunIlluminance, 0.0f);
        data.Update[0]    = stride;
        data.Update[1]    = offset;
        data.Update[2]    = m_NodeCount;
        data.Update[3]    = m_TriangleCount;
        data.Params       = Vec4(params.Sky, params.Bounce, Maths::Max(0.005f, minSpacing * 0.01f), params.EmissiveScale);
        data.Scales       = Vec4(params.LightScale, 2.0f * maxSpacing, 0.2f * minSpacing, 0.0f);
        data.Extra[0]     = m_LightCount;
        data.Extra[1]     = perProbe < kSettleUpdates ? 1u : 0u;
        data.Extra[2]     = params.LightRange ? 1u : 0u;
        data.Extra[3]     = 0u;
        m_UpdatesSinceReset++;

        m_TraceSet->SetUniformBufferData(0, &data);
        m_TraceSet->SetStorageBuffer(1, m_NodeBuffer);
        m_TraceSet->SetStorageBuffer(2, m_TriangleBuffer);
        m_TraceSet->SetStorageBuffer(3, m_MaterialBuffer);
        m_TraceSet->SetStorageBuffer(4, m_ProbeBuffer);
        m_TraceSet->SetTexture(5, environmentMap, 0, TextureType::CUBE);
        m_TraceSet->SetStorageBuffer(6, m_LightBuffer);
        m_TraceSet->Update();
        m_TraceSet->TransitionImages(commandBuffer);

        PipelineDesc desc {};
        desc.shader    = m_TraceShader;
        desc.DebugName = "GIProbeTrace";
        auto pipeline  = Pipeline::Get(desc);
        commandBuffer->BindPipeline(pipeline);

        auto set = m_TraceSet.get();
        Renderer::BindDescriptorSets(pipeline.get(), commandBuffer, 0, &set, 1);
        Renderer::GetRenderer()->Dispatch(commandBuffer, groups, 1, 1);

        commandBuffer->BufferMemoryBarrier(CommandBuffer::PipelineStage::COMPUTE_SHADER, CommandBuffer::PipelineStage::FRAGMENT_SHADER);
    }
}
