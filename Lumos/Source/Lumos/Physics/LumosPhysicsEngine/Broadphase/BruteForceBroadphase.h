#pragma once

#include "Broadphase.h"
#include "Maths/Vector3.h"

namespace Lumos
{
    struct Arena;
    class LUMOS_EXPORT BruteForceBroadphase : public Broadphase
    {
    public:
        explicit BruteForceBroadphase(const Vec3& axis = Vec3(0.0f));
        virtual ~BruteForceBroadphase();

        void FindPotentialCollisionPairs(RigidBody3D* rootObject, TDArray<CollisionPair>& collisionPairs, uint32_t totalRigidBodyCount) override;
        void DebugDraw() override;

    private:
        Vec3 m_axis;
        Arena* m_Arena = nullptr;
    };
}
