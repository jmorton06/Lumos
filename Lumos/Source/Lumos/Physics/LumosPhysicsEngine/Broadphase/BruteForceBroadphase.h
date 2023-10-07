#pragma once

#include "Broadphase.h"
#include <glm/vec3.hpp>

namespace Lumos
{

    class LUMOS_EXPORT BruteForceBroadphase : public Broadphase
    {
    public:
        explicit BruteForceBroadphase(const glm::vec3& axis = glm::vec3(0.0f));
        virtual ~BruteForceBroadphase();

        void FindPotentialCollisionPairs(RigidBody3D* rootObject, std::vector<CollisionPair>& collisionPairs) override;
        void DebugDraw() override;

    private:
        glm::vec3 m_axis;
    };
}
