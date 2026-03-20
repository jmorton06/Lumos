#pragma once
#include "Maths/Vector3.h"

namespace Lumos
{
    class RigidBody3D;
    class CollisionShape;

    struct RaycastHit
    {
        RigidBody3D* Body     = nullptr;
        CollisionShape* Shape = nullptr;
        Vec3 Point            = Vec3(0.0f);
        Vec3 Normal           = Vec3(0.0f);
        float Distance        = 0.0f;

        bool Hit() const { return Body != nullptr; }
        operator bool() const { return Hit(); }
    };

    struct RaycastQuery
    {
        Vec3 Origin;
        Vec3 Direction;
        float MaxDistance     = 1000.0f;
        uint16_t LayerMask    = 0xFFFF;
        bool HitTriggers      = false;

        RaycastQuery() = default;
        RaycastQuery(const Vec3& origin, const Vec3& dir, float maxDist = 1000.0f)
            : Origin(origin)
            , Direction(dir.Normalised())
            , MaxDistance(maxDist)
        {
        }
    };
}
