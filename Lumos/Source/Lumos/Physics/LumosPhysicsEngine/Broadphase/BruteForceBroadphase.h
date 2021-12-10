#pragma once

#include "Broadphase.h"
#include "Maths/Maths.h"

namespace Lumos
{

    class LUMOS_EXPORT BruteForceBroadphase : public Broadphase
    {
    public:
        explicit BruteForceBroadphase(const glm::vec3& axis = glm::vec3(0.0f));
        virtual ~BruteForceBroadphase();

        void FindPotentialCollisionPairs(RigidBody3D** objects, uint32_t objectCount, std::vector<CollisionPair>& collisionPairs) override;
        void DebugDraw() override;

    private:
        glm::vec3 m_axis;
    };
}
