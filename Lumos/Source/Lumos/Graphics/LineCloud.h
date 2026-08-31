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

        // One line segment, packed as 4 vec4 to match LineCloud.vert's SSBO.
        struct LineCloudSegment
        {
            Vec4 A;      // xyz = endpoint A, w = width at A
            Vec4 B;      // xyz = endpoint B, w = arc-length at A (for continuous dashing)
            Vec4 Colour; // rgba
            Vec4 Params; // x = dash period (world units, 0 = solid), y = duty (0..1),
                         // z = width at B (0 = same as A) for perspective-tapered long segments
        };

        class LineCloud
        {
        public:
            LineCloud() = default;
            ~LineCloud();

            void AddLine(const Vec3& a, const Vec3& b, const Vec4& colour, float width,
                         float dashPeriod = 0.0f, float duty = 0.5f, float arcAtA = 0.0f,
                         float widthB = 0.0f);
            void Finish();
            void Clear();
            void ReleaseBuffer(); // free GPU buffer while the device is still alive

            StorageBuffer* GetBuffer() const { return m_Buffer; }
            uint32_t GetCount() const { return m_Count; } // segment count

            void SetLocal(bool local) { m_Local = local; }
            bool IsLocal() const { return m_Local; }

        private:
            TDArray<LineCloudSegment> m_Segments;
            StorageBuffer* m_Buffer = nullptr;
            uint32_t m_Count        = 0;
            bool m_Local            = false;
        };

        // Engine-generic registry of line clouds to draw each frame (like PointCloud).
        TDArray<SharedPtr<LineCloud>>& GetActiveLineClouds();
        void RegisterLineCloud(const SharedPtr<LineCloud>& cloud);
        void ClearLineClouds();

        // arcAtA: distance along the path at point A. Dash phase is computed from
        // it, so a multi-segment path must pass a running total or every segment
        // restarts the pattern (which reads as "all on" once segments are short).
        void DrawImmediateLine(const Vec3& a, const Vec3& b, const Vec4& colour, float width,
                               bool local = false, float dashPeriod = 0.0f, float duty = 0.5f,
                               float widthB = 0.0f, float arcAtA = 0.0f);
        LineCloud* GetImmediateLineCloud(bool local);
        void ResetImmediateLineClouds();   // per-frame segment clear (end of pass)
        void ReleaseImmediateLineClouds(); // teardown: free buffers before device dies
    }
}
