#pragma once
#include "Maths/Vector3.h"

namespace Lumos
{
    namespace Maths
    {
        class BoundingBox;
        class Ray
        {
        public:
            Ray();
            Ray(const Vec3& origin, const Vec3& direction);

            bool Intersects(const BoundingBox& bb) const;
            bool Intersects(const BoundingBox& bb, float& t) const;
            bool IntersectsTriangle(const Vec3& a, const Vec3& b, const Vec3& c, float& t) const;

            Vec3 Origin, Direction;
        };
    }
}
