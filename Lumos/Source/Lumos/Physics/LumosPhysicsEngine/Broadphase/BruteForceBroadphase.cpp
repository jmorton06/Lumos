#include "Precompiled.h"
#include "BruteForceBroadphase.h"

namespace Lumos
{

    BruteForceBroadphase::BruteForceBroadphase(const glm::vec3& axis)
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
        LUMOS_PROFILE_FUNCTION();
        if(objectCount == 0)
            return;

        for(size_t i = 0; i < objectCount - 1; ++i)
        {
            for(size_t j = i + 1; j < objectCount; ++j)
            {
                RigidBody3D* obj1 = objects[i];
                RigidBody3D* obj2 = objects[j];

                if(!obj1->GetCollisionShape() || !obj2->GetCollisionShape())
                    continue;

                // Skip pairs of two at objects at rest
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

                CollisionPair pair;

                if(obj1 < obj2)
                {
                    pair.pObjectA = obj1;
                    pair.pObjectB = obj2;
                }
                else
                {
                    pair.pObjectA = obj2;
                    pair.pObjectB = obj1;
                }

                bool duplicate = false;
                for(int i = 0; i < collisionPairs.size(); i++)
                {
                    auto& pair2 = collisionPairs[i];

                    if(pair.pObjectA == pair2.pObjectA && pair.pObjectB == pair2.pObjectB)
                    {
                        duplicate = true;
                    }
                }
                if(!duplicate)
                    collisionPairs.push_back(pair);
            }
        }
    }

    void BruteForceBroadphase::DebugDraw()
    {
    }
}
