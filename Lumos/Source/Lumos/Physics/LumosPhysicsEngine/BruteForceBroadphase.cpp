#include "Precompiled.h"
#include "BruteForceBroadphase.h"

namespace Lumos
{

    BruteForceBroadphase::BruteForceBroadphase(const Maths::Vector3& axis)
        : Broadphase()
        , m_axis(axis)
    {
    }

    BruteForceBroadphase::~BruteForceBroadphase()
    {
    }

    void BruteForceBroadphase::FindPotentialCollisionPairs(RigidBody3D** objects, uint32_t objectCount,
        std::vector<CollisionPair>& collisionPairs)
    {
        if(objectCount == 0)
            return;

        for(size_t i = 0; i < objectCount - 1; ++i)
        {
            for(size_t j = i + 1; j < objectCount; ++j)
            {
                RigidBody3D* obj1 = objects[i];
                RigidBody3D* obj2 = objects[j];

                if(obj1->GetIsAtRest() && obj2->GetIsAtRest())
                    continue;

                // Skip pairs of two at static objects
                if(obj1->GetIsStatic() && obj2->GetIsStatic())
                    continue;

                // Skip pairs of one static and one at rest
                if(obj1->GetIsAtRest() && obj2->GetIsStatic())
                    continue;

                if(obj1->GetIsStatic() && obj2->GetIsAtRest())
                    continue;

                // Check they both have collision shapes and at least one is awake
                if(obj1->GetCollisionShape() && obj2->GetCollisionShape() && (obj1->IsAwake() || obj2->IsAwake()))
                {
                    CollisionPair cp;
                    cp.pObjectA = obj1;
                    cp.pObjectB = obj2;

                    collisionPairs.push_back(cp);
                }
            }
        }
    }

    void BruteForceBroadphase::DebugDraw()
    {
    }
}
