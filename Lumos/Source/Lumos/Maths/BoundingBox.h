#pragma once
#include "Maths/MathsFwd.h"
#include "Maths/Vector3.h"
namespace Lumos
{
    namespace Maths
    {
        enum Intersection : u8;
        class BoundingSphere;
        class Rect;
        class BoundingBox
        {
            friend class BoundingSphere;

        public:
            BoundingBox();
            BoundingBox(const Vec3& min, const Vec3& max);
            BoundingBox(const Vec3* points, uint32_t numPoints);
            BoundingBox(const Rect& rect, const Vec3& center = Vec3(0.0f));
            BoundingBox(const BoundingBox& other);
            BoundingBox(BoundingBox&& other);
            ~BoundingBox();

            void Clear();

            BoundingBox& operator=(const BoundingBox& other);
            BoundingBox& operator=(BoundingBox&& other);

            void Set(const Vec3& min, const Vec3& max);
            void Set(const Vec3* points, uint32_t numPoints);

            void SetFromPoints(const Vec3* points, uint32_t numPoints);
            void SetFromPoints(const Vec3* points, uint32_t numPoints, const Mat4& transform);

            void SetFromTransformedAABB(const BoundingBox& aabb, const Mat4& transform);

            void Translate(const Vec3& translation);
            void Translate(float x, float y, float z);

            void Scale(const Vec3& scale);
            void Scale(float x, float y, float z);

            void Rotate(const Mat3& rotation);

            void Transform(const Mat4& transform);
            BoundingBox Transformed(const Mat4& transform) const;

            void Merge(const BoundingBox& other);
            void Merge(const Vec3& point);

            void Merge(const BoundingBox& other, const Mat4& transform);
            void Merge(const Vec3& point, const Mat4& transform);

            void Merge(const BoundingBox& other, const Mat3& transform);
            void Merge(const Vec3& point, const Mat3& transform);

            void ExtendToCube();

            Intersection IsInside(const Vec3& point) const;
            Intersection IsInside(const BoundingBox& box) const;
            Intersection IsInside(const BoundingSphere& sphere) const;

            bool IsInsideFast(const BoundingBox& box) const;

            Vec3 Size() const;
            Vec3 Center() const;
            Vec3 Min() const;
            Vec3 Max() const;

            Vec3 GetExtents() const { return m_Max - m_Min; }

            Vec3 m_Min;
            Vec3 m_Max;
        };
    }
}
