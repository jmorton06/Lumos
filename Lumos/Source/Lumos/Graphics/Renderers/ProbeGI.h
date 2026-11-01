#pragma once
#include "Core/Core.h"
#include "Maths/Vector3.h"
#include "Core/DataStructures/TDArray.h"

namespace Lumos
{
    class Scene;

    namespace Graphics
    {
        class Shader;
        class DescriptorSet;
        class StorageBuffer;
        class CommandBuffer;
        class Texture;
        class Material;

        struct ProbeGIParams
        {
            float Spacing    = 2.0f;
            float Hysteresis = 0.97f;
            float Bounce     = 1.0f;
            float Sky        = 1.0f;
            u32 UpdateStride = 4;
            float LightScale    = 1.0f; // light intensity -> environment map units
            float EmissiveScale = 1.0f; // material emissive (as drawn, after exposure) -> environment map units
            bool LightRange     = false;
        };

        // A grid of irradiance probes over the static scene. Each frame a share of them fire rays
        // through a BVH of the scene's triangles (compute, no ray tracing hardware), light the hits
        // with the sun, point / spot lights, emissive materials, the sky and the grid itself, and
        // fold the result into L1 SH. The same rays keep an octahedral depth map per probe (visibility
        // when sampling) and move probes out of walls while they settle.
        // Static = models without an animated mesh or a moving rigid body; they are re-read when the
        // set of them, or where they are, changes.
        class ProbeGI
        {
        public:
            ProbeGI();
            ~ProbeGI();

            // Start of the frame, before the forward pass's descriptors are written
            void Prepare(Scene* scene, bool enabled, const ProbeGIParams& params);
            // the scene setting, apart from frames that skip GI: turning it back on starts the probes over
            void SetEnabled(bool enabled);
            // Outside a render pass. sunIlluminance is in the environment maps' units.
            void Update(CommandBuffer* commandBuffer, Texture* environmentMap, const Vec3& sunDirection, const Vec3& sunIlluminance, bool hasSun, const ProbeGIParams& params);

            bool IsActive() const { return m_Active; }
            // always a valid buffer, so the forward pass's binding is satisfied with GI off
            StorageBuffer* GetProbeBuffer() const { return m_ProbeBuffer; }
            const Vec3& GetOrigin() const { return m_Origin; }
            const Vec3& GetSpacing() const { return m_Spacing; }
            const u32* GetCounts() const { return m_Counts; }
            u32 GetProbeCount() const { return m_Counts[0] * m_Counts[1] * m_Counts[2]; }
            u32 GetTriangleCount() const { return m_TriangleCount; }

            void Invalidate() { m_Signature = 0; }

        private:
            u64 ComputeSignature(Scene* scene, float spacing) const;
            void Rebuild(Scene* scene, float spacing);

            SharedPtr<Shader> m_TraceShader;
            SharedPtr<Shader> m_AlbedoShader;
            SharedPtr<DescriptorSet> m_TraceSet;
            TDArray<SharedPtr<DescriptorSet>> m_AlbedoSets; // one per material with a map

            StorageBuffer* m_NodeBuffer     = nullptr;
            StorageBuffer* m_TriangleBuffer = nullptr;
            StorageBuffer* m_MaterialBuffer = nullptr;
            StorageBuffer* m_ProbeBuffer    = nullptr;
            StorageBuffer* m_LightBuffer    = nullptr;

            TDArray<Material*> m_Materials;
            bool m_AlbedoPending = false;

            Vec3 m_Origin  = Vec3(0.0f);
            Vec3 m_Spacing = Vec3(1.0f);
            u32 m_Counts[3] = { 1, 1, 1 };
            u32 m_NodeCount     = 0;
            u32 m_TriangleCount = 0;

            u64 m_Signature        = 0;
            u64 m_PendingSignature = 0;
            u32 m_StableFrames     = 0;
            u32 m_FrameCounter     = 0;
            u32 m_UpdatesSinceReset = 0;
            u32 m_LastStride        = 4;
            u32 m_LightCount        = 0;
            bool m_Enabled          = false;
            bool m_Active          = false;
            bool m_Supported       = false;
        };
    }
}
