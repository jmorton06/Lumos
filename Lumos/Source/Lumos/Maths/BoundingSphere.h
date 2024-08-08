#pragma once
#include "Maths/MathsFwd.h"
#include "Maths/Vector3.h"

namespace Lumos
{
    namespace Maths
    {
        class BoundingBox;
        class Frustum;

        class BoundingSphere
        {
            friend class Frustum;
            friend class BoundingBox;

        public:
            BoundingSphere();
            BoundingSphere(const Vec3& center, float radius);
            BoundingSphere(const Vec3* points, unsigned int count);
            BoundingSphere(const Vec3* points, unsigned int count, const Vec3& center);
            BoundingSphere(const Vec3* points, unsigned int count, const Vec3& center, float radius);
            BoundingSphere(const BoundingSphere& other);
            BoundingSphere(BoundingSphere&& other);
            BoundingSphere& operator=(const BoundingSphere& other);
            BoundingSphere& operator=(BoundingSphere&& other);
            ~BoundingSphere() = default;

            const Vec3& GetCenter() const;
            float GetRadius() const;

            void SetCenter(const Vec3& center);
            void SetRadius(float radius);

            bool IsInside(const Vec3& point) const;
            bool IsInside(const BoundingSphere& sphere) const;
            bool IsInside(const BoundingBox& box) const;
            bool IsInside(const Frustum& frustum) const;

            bool Contains(const Vec3& point) const;
            bool Contains(const BoundingSphere& other) const;
            bool Intersects(const BoundingSphere& other) const;
            bool Intersects(const Vec3& point) const;
            bool Intersects(const Vec3& point, float radius) const;

            void Merge(const BoundingSphere& other);
            void Merge(const Vec3& point);
            void Merge(const Vec3* points, unsigned int count);

            void Transform(const Mat4& transform);

        private:
            Vec3 m_Center;
            float m_Radius;
        };
    }
}
