#pragma once
#include "Maths/Vector3.h"
#include "Maths/Vector4.h"
#include "Core/DataStructures/TDArray.h"
#include "Core/Reference.h"

namespace Lumos
{
    namespace Graphics
    {
        class StorageBuffer;

        // One renderable point. Packed as 2 vec4 to match PointCloud.vert's SSBO.
        struct PointCloudVertex
        {
            Vec4 PosSize; // xyz = world position, w = size
            Vec4 Colour;  // rgba
        };

        class PointCloud
        {
        public:
            PointCloud() = default;
            ~PointCloud();

            // sourceIndices (optional): for each point, the index it came from in
            // whatever the caller built the cloud out of - a CSV row, an entry in a
            // catalogue. A builder that FILTERS (skipping faint stars, say) makes the
            // point index and the source index diverge, and every pick result would
            // then name the wrong thing. Pass the mapping and picking hands back the
            // caller's own index; pass nothing and the two are identical.
            void SetPoints(const PointCloudVertex* points, uint32_t count, const uint32_t* sourceIndices = nullptr);

            StorageBuffer* GetBuffer() const { return m_Buffer; }
            uint32_t GetCount() const { return m_Count; }

            // Both return a SOURCE index (see SetPoints), or -1 for no hit.
            int PickRay(const Vec3& origin, const Vec3& dir, float maxTan,
                        float* outTan = nullptr, float distWeight = 0.0f) const;

            int NearestPoint(const Vec3& pos, float* outDist = nullptr) const;

            // Source index -> point index, or -1. Linear; cache the answer.
            int PointIndexForSource(int source) const;

            // Point index -> source index. Identity when no mapping was given.
            int SourceIndex(uint32_t point) const
            {
                if(m_SourceIndices.Empty())
                    return (int)point;
                return point < m_SourceIndices.Size() ? (int)m_SourceIndices[point] : -1;
            }

        private:
            StorageBuffer* m_Buffer = nullptr;
            uint32_t m_Count        = 0;
            TDArray<Vec4> m_Positions;         // CPU copy (xyz + size in w) for picking/queries
            TDArray<uint32_t> m_SourceIndices; // point -> caller's index; empty = identity
        };

        TDArray<SharedPtr<PointCloud>>& GetActivePointClouds();
        void RegisterPointCloud(const SharedPtr<PointCloud>& cloud);
        void ClearPointClouds();
        void SetPointCloudSkyFade(float f); // global star-field brightness (1 = normal)
        float GetPointCloudSkyFade();
        void SetPointCloudStreak(const Vec3& dir, float amount);
        Vec4 GetPointCloudStreak(); // xyz dir, w amount
        // x = diffraction spike strength (0 = off), y = spike length in sigmas,
        // z = Gaussian sigma floor in pixels (steadier but softer as it rises)
        void SetPointCloudLook(float spike, float spikeLen, float sigmaFloor);
        Vec4 GetPointCloudLook();
        // One point drawn at zero brightness (the star a 3D sphere is standing in
        // for). Source index as passed to SetPoints; -1 = none.
        void SetPointCloudHiddenSource(int source);
        int GetPointCloudHiddenIndex(); // point index, -1 = none
    }
}
