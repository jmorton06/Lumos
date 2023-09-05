#pragma once
#include "Maths/Rect.h"
#include "MathsUtilities.h"
#include <glm/vec3.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>

namespace Lumos
{
    namespace Maths
    {

        class BoundingSphere;
        class BoundingBox
        {
            friend class BoundingSphere;

        public:
            BoundingBox();
            BoundingBox(const glm::vec3& min, const glm::vec3& max);
            BoundingBox(const glm::vec3* points, uint32_t numPoints);
            BoundingBox(const Rect& rect, const glm::vec3& center = glm::vec3(0.0f));
            BoundingBox(const BoundingBox& other);
            BoundingBox(BoundingBox&& other);
            ~BoundingBox();

            void Clear();

            BoundingBox& operator=(const BoundingBox& other);
            BoundingBox& operator=(BoundingBox&& other);

            void Set(const glm::vec3& min, const glm::vec3& max);
            void Set(const glm::vec3* points, uint32_t numPoints);

            void SetFromPoints(const glm::vec3* points, uint32_t numPoints);
            void SetFromPoints(const glm::vec3* points, uint32_t numPoints, const glm::mat4& transform);

            void SetFromTransformedAABB(const BoundingBox& aabb, const glm::mat4& transform);

            void Translate(const glm::vec3& translation);
            void Translate(float x, float y, float z);

            void Scale(const glm::vec3& scale);
            void Scale(float x, float y, float z);

            void Rotate(const glm::mat3& rotation);

            void Transform(const glm::mat4& transform);
            BoundingBox Transformed(const glm::mat4& transform) const;

            void Merge(const BoundingBox& other);
            void Merge(const glm::vec3& point);

            void Merge(const BoundingBox& other, const glm::mat4& transform);
            void Merge(const glm::vec3& point, const glm::mat4& transform);

            void Merge(const BoundingBox& other, const glm::mat3& transform);
            void Merge(const glm::vec3& point, const glm::mat3& transform);

            Intersection IsInside(const glm::vec3& point) const;
            Intersection IsInside(const BoundingBox& box) const;
            Intersection IsInside(const BoundingSphere& sphere) const;

            bool IsInsideFast(const BoundingBox& box) const;

            glm::vec3 Size() const;
            glm::vec3 Center() const;
            glm::vec3 Min() const;
            glm::vec3 Max() const;

            glm::vec3 GetExtents() const { return m_Max - m_Min; }

            glm::vec3 m_Min;
            glm::vec3 m_Max;
        };
    }
}
