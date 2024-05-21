#pragma once
#include <glm/ext/vector_float3.hpp>

namespace Lumos
{
    namespace Maths
    {
        class BoundingBox;
        class Ray
        {
        public:
            Ray();
            Ray(const glm::vec3& origin, const glm::vec3& direction);

            bool Intersects(const BoundingBox& bb) const;
            bool Intersects(const BoundingBox& bb, float& t) const;
            bool IntersectsTriangle(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, float& t) const;

            glm::vec3 Origin, Direction;
        };
    }
}
