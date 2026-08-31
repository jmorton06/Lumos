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

            void SetPoints(const PointCloudVertex* points, uint32_t count);

            StorageBuffer* GetBuffer() const { return m_Buffer; }
            uint32_t GetCount() const { return m_Count; }

            int PickRay(const Vec3& origin, const Vec3& dir, float maxTan,
                        float* outTan = nullptr, float distWeight = 0.0f) const;

            int NearestPoint(const Vec3& pos, float* outDist = nullptr) const;

        private:
            StorageBuffer* m_Buffer = nullptr;
            uint32_t m_Count        = 0;
            TDArray<Vec4> m_Positions; // CPU copy (xyz + size in w) for picking/queries
        };

        TDArray<SharedPtr<PointCloud>>& GetActivePointClouds();
        void RegisterPointCloud(const SharedPtr<PointCloud>& cloud);
        void ClearPointClouds();
        void SetPointCloudSkyFade(float f); // global star-field brightness (1 = normal)
        float GetPointCloudSkyFade();
        void SetPointCloudStreak(const Vec3& dir, float amount);
        Vec4 GetPointCloudStreak(); // xyz dir, w amount
    }
}
