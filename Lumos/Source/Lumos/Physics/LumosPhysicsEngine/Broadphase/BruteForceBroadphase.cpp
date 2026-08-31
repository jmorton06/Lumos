#include "Precompiled.h"
#include "BruteForceBroadphase.h"
#include "Core/DataStructures/Set.h"

namespace Lumos
{

    BruteForceBroadphase::BruteForceBroadphase(const Vec3& axis)
        : Broadphase()
        , m_axis(axis)
    {
        m_Arena = ArenaAlloc(Megabytes(1));
    }

    BruteForceBroadphase::~BruteForceBroadphase()
    {
        if(m_Arena)
            ArenaRelease(m_Arena);
    }

    void BruteForceBroadphase::FindPotentialCollisionPairs(RigidBody3D* rootObject,
                                                           TDArray<CollisionPair>& collisionPairs, uint32_t totalRigidBodyCount)
    {
        LUMOS_PROFILE_FUNCTION();


        // Iterate through all objects using array indexing instead of linked list
        for(uint32_t i = 0; i < totalRigidBodyCount; i++)
        {
            RigidBody3D& obj1 = rootObject[i];
            
            if(!obj1.GetIsValid() || !obj1.GetCollisionShape())
                continue;

            // Only check against objects after this one to avoid duplicate pairs
            for(uint32_t j = i + 1; j < totalRigidBodyCount; j++)
            {
                RigidBody3D& obj2 = rootObject[j];
                
                if(!obj2.GetIsValid() || !obj2.GetCollisionShape())
                    continue;

                // Skip pairs of two static objects
                if(obj1.GetIsStatic() && obj2.GetIsStatic())
                    continue;

                // Skip pairs of two non-static objects that are both at rest
                // (Don't skip static-dynamic pairs even if dynamic object is at rest,
                // because another dynamic object could push the at-rest object into the static one)
                if(obj1.GetIsAtRest() && obj2.GetIsAtRest() && !obj1.GetIsStatic() && !obj2.GetIsStatic())
                    continue;

                // Skip pairs filtered out by collision layers
                if(!obj1.CanCollideWith(&obj2))
                    continue;

                CollisionPair pair;
                // Order consistently so narrowphase/manifold lookups are stable.
                if(&obj1 < &obj2)
                {
                    pair.pObjectA = &obj1;
                    pair.pObjectB = &obj2;
                }
                else
                {
                    pair.pObjectA = &obj2;
                    pair.pObjectB = &obj1;
                }

                collisionPairs.EmplaceBack(pair);
            }
        }
    }

    void BruteForceBroadphase::DebugDraw()
    {
    }
}
