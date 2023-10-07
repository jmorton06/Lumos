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

    void BruteForceBroadphase::FindPotentialCollisionPairs(RigidBody3D* rootObject,
                                                           std::vector<CollisionPair>& collisionPairs)
    {
        LUMOS_PROFILE_FUNCTION();

        RigidBody3D* current = rootObject;
        while(current)
        {
            if(current->GetCollisionShape())
            {
                RigidBody3D* current2 = rootObject;
                {
                    while(current2)
                    {
                        if(current2->GetCollisionShape())
                        {
                            RigidBody3D* obj1 = current;
                            RigidBody3D* obj2 = current2;

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
            }
        }
    }

    void BruteForceBroadphase::DebugDraw()
    {
    }
}
