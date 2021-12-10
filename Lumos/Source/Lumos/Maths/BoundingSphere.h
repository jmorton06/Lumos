#pragma once
#include <glm/glm.hpp>

namespace Lumos
{
    class BoundingBox;
    class Frustum;

    class BoundingSphere
    {
        friend class Frustum;
        friend class BoundingBox;

    public:
        BoundingSphere();
        BoundingSphere(const glm::vec3& center, float radius);
        BoundingSphere(const glm::vec3* points, unsigned int count);
        BoundingSphere(const glm::vec3* points, unsigned int count, const glm::vec3& center);
        BoundingSphere(const glm::vec3* points, unsigned int count, const glm::vec3& center, float radius);
        BoundingSphere(const BoundingSphere& other);
        BoundingSphere(BoundingSphere&& other);
        BoundingSphere& operator=(const BoundingSphere& other);
        BoundingSphere& operator=(BoundingSphere&& other);
        ~BoundingSphere() = default;

        const glm::vec3& GetCenter() const;
        float GetRadius() const;

        void SetCenter(const glm::vec3& center);
        void SetRadius(float radius);

        bool IsInside(const glm::vec3& point) const;
        bool IsInside(const BoundingSphere& sphere) const;
        bool IsInside(const BoundingBox& box) const;
        bool IsInside(const Frustum& frustum) const;

        bool Contains(const glm::vec3& point) const;
        bool Contains(const BoundingSphere& other) const;
        bool Intersects(const BoundingSphere& other) const;
        bool Intersects(const glm::vec3& point) const;
        bool Intersects(const glm::vec3& point, float radius) const;

        void Merge(const BoundingSphere& other);
        void Merge(const glm::vec3& point);
        void Merge(const glm::vec3* points, unsigned int count);

        void Transform(const glm::mat4& transform);

    private:
        glm::vec3 m_Center;
        float m_Radius;
    };
}
