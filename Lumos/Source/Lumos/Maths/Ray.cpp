#include "Precompiled.h"
#include "Ray.h"
#include "BoundingBox.h"
#include "Maths/MathsUtilities.h"

namespace Lumos
{
    namespace Maths
    {
        Ray::Ray()
        {
            Origin    = Vec3(0.0f);
            Direction = Vec3(0.0f);
        }

        Ray::Ray(const Vec3& origin, const Vec3& direction)
        {
            Origin    = origin;
            Direction = direction;
        }

        bool Ray::Intersects(const BoundingBox& bb, float& t) const
        {
            LUMOS_PROFILE_FUNCTION();
            t = 0.0f;
            // Check for ray origin being inside the bb
            if(bb.IsInside(Origin))
                return true;

            float dist = Maths::M_INFINITY;

            // Check for intersecting in the X-direction
            if(Origin.x < bb.Min().x && Direction.x > 0.0f)
            {
                float x = (bb.Min().x - Origin.x) / Direction.x;
                if(x < dist)
                {
                    Vec3 point = Origin + x * Direction;
                    if(point.y >= bb.Min().y && point.y <= bb.Max().y && point.z >= bb.Min().z && point.z <= bb.Max().z)
                        dist = x;
                }
            }
            if(Origin.x > bb.Max().x && Direction.x < 0.0f)
            {
                float x = (bb.Max().x - Origin.x) / Direction.x;
                if(x < dist)
                {
                    Vec3 point = Origin + x * Direction;
                    if(point.y >= bb.Min().y && point.y <= bb.Max().y && point.z >= bb.Min().z && point.z <= bb.Max().z)
                        dist = x;
                }
            }
            // Check for intersecting in the Y-direction
            if(Origin.y < bb.Min().y && Direction.y > 0.0f)
            {
                float x = (bb.Min().y - Origin.y) / Direction.y;
                if(x < dist)
                {
                    Vec3 point = Origin + x * Direction;
                    if(point.x >= bb.Min().x && point.x <= bb.Max().x && point.z >= bb.Min().z && point.z <= bb.Max().z)
                        dist = x;
                }
            }
            if(Origin.y > bb.Max().y && Direction.y < 0.0f)
            {
                float x = (bb.Max().y - Origin.y) / Direction.y;
                if(x < dist)
                {
                    Vec3 point = Origin + x * Direction;
                    if(point.x >= bb.Min().x && point.x <= bb.Max().x && point.z >= bb.Min().z && point.z <= bb.Max().z)
                        dist = x;
                }
            }
            // Check for intersecting in the Z-direction
            if(Origin.z < bb.Min().z && Direction.z > 0.0f)
            {
                float x = (bb.Min().z - Origin.z) / Direction.z;
                if(x < dist)
                {
                    Vec3 point = Origin + x * Direction;
                    if(point.x >= bb.Min().x && point.x <= bb.Max().x && point.y >= bb.Min().y && point.y <= bb.Max().y)
                        dist = x;
                }
            }
            if(Origin.z > bb.Max().z && Direction.z < 0.0f)
            {
                float x = (bb.Max().z - Origin.z) / Direction.z;
                if(x < dist)
                {
                    Vec3 point = Origin + x * Direction;
                    if(point.x >= bb.Min().x && point.x <= bb.Max().x && point.y >= bb.Min().y && point.y <= bb.Max().y)
                        dist = x;
                }
            }

            t = dist;
            return dist < Maths::M_INFINITY;
            ;
        }

        bool Ray::Intersects(const BoundingBox& bb) const
        {
            float distance;
            return Intersects(bb, distance);
        }

        bool Ray::IntersectsTriangle(const Vec3& a, const Vec3& b, const Vec3& c, float& t) const
        {
            LUMOS_PROFILE_FUNCTION();
            const Vec3 E1      = b - a;
            const Vec3 E2      = c - a;
            const Vec3 N       = E1.Cross(E2);
            const float det    = -Maths::Dot(Direction, N);
            const float invdet = 1.f / det;
            const Vec3 AO      = Origin - a;
            const Vec3 DAO     = Maths::Cross(AO, Direction);
            const float u      = Maths::Dot(E2, DAO) * invdet;
            const float v      = -Maths::Dot(E1, DAO) * invdet;
            t                  = Maths::Dot(AO, N) * invdet;
            return (det >= 1e-6f && t >= 0.0f && u >= 0.0f && v >= 0.0f && (u + v) <= 1.0f);
        }
    }
}
