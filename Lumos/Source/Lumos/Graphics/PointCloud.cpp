#include "Precompiled.h"
#include "PointCloud.h"
#include "Graphics/RHI/StorageBuffer.h"
#include <cmath>

namespace Lumos
{
    namespace Graphics
    {
        PointCloud::~PointCloud()
        {
            delete m_Buffer;
        }

        void PointCloud::SetPoints(const PointCloudVertex* points, uint32_t count)
        {
            m_Count = count;
            m_Positions.Clear();
            if(count == 0)
                return;

            m_Positions.Reserve(count);
            for(uint32_t i = 0; i < count; i++)
                m_Positions.PushBack(points[i].PosSize);

            uint32_t size = count * sizeof(PointCloudVertex);
            if(!m_Buffer)
                m_Buffer = StorageBuffer::Create(size, points);
            else
                m_Buffer->Resize(size, points);
        }

        int PointCloud::PickRay(const Vec3& origin, const Vec3& dir, float maxTan, float* outTan, float distWeight) const
        {
            int best         = -1;
            float maxTan2    = maxTan * maxTan;
            float bestScore  = 1e30f;
            float bestTan2   = maxTan2;
            uint32_t n       = (uint32_t)m_Positions.Size();
            for(uint32_t i = 0; i < n; i++)
            {
                const Vec4& p = m_Positions[i];
                float vx = p.x - origin.x, vy = p.y - origin.y, vz = p.z - origin.z;
                float t  = vx * dir.x + vy * dir.y + vz * dir.z;
                if(t <= 0.0001f)
                    continue; // behind the camera

                // Perpendicular distance from point to ray, as tan^2 of the angle.
                float px = vx - dir.x * t, py = vy - dir.y * t, pz = vz - dir.z * t;
                float tan2 = (px * px + py * py + pz * pz) / (t * t);
                if(tan2 >= maxTan2)
                    continue; // outside the click radius

                float score = tan2 * (1.0f + t * distWeight);
                if(score < bestScore)
                {
                    bestScore = score;
                    bestTan2  = tan2;
                    best      = (int)i;
                }
            }
            if(outTan && best >= 0)
                *outTan = sqrtf(bestTan2);
            return best;
        }

        int PointCloud::NearestPoint(const Vec3& pos, float* outDist) const
        {
            int best        = -1;
            float bestDist2 = 1e30f;
            uint32_t n      = (uint32_t)m_Positions.Size();
            for(uint32_t i = 0; i < n; i++)
            {
                const Vec4& p = m_Positions[i];
                float dx = p.x - pos.x, dy = p.y - pos.y, dz = p.z - pos.z;
                float d2 = dx * dx + dy * dy + dz * dz;
                if(d2 < bestDist2)
                {
                    bestDist2 = d2;
                    best      = (int)i;
                }
            }
            if(outDist && best >= 0)
                *outDist = sqrtf(bestDist2);
            return best;
        }

        static TDArray<SharedPtr<PointCloud>> s_ActivePointClouds;

        TDArray<SharedPtr<PointCloud>>& GetActivePointClouds()
        {
            return s_ActivePointClouds;
        }

        void RegisterPointCloud(const SharedPtr<PointCloud>& cloud)
        {
            s_ActivePointClouds.PushBack(cloud);
        }

        void ClearPointClouds()
        {
            s_ActivePointClouds.Clear();
        }

        static float s_PointCloudSkyFade = 1.0f;
        void SetPointCloudSkyFade(float f) { s_PointCloudSkyFade = f; }
        float GetPointCloudSkyFade() { return s_PointCloudSkyFade; }

        // Warp streak state (see header). Zero amount = inert.
        static Vec4 s_PointCloudStreak = Vec4(0.0f, 0.0f, 1.0f, 0.0f);
        void SetPointCloudStreak(const Vec3& dir, float amount)
        {
            s_PointCloudStreak = Vec4(dir, amount);
        }
        Vec4 GetPointCloudStreak() { return s_PointCloudStreak; }
    }
}
