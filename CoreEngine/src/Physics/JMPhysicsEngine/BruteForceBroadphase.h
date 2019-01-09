#pragma once

#include "JM.h"
#include "Broadphase.h"

namespace jm
{

	class JM_EXPORT BruteForceBroadphase : public Broadphase
	{
	public:
		BruteForceBroadphase();
		virtual ~BruteForceBroadphase();

		void FindPotentialCollisionPairs(std::vector<std::shared_ptr<PhysicsObject3D>> objects, std::vector<CollisionPair> &collisionPairs) override;
		void DebugDraw() override;
	};
}
