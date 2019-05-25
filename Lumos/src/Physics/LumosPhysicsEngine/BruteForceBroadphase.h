#pragma once

#include "LM.h"
#include "Broadphase.h"

namespace lumos
{

	class LUMOS_EXPORT BruteForceBroadphase : public Broadphase
	{
	public:
		BruteForceBroadphase();
		virtual ~BruteForceBroadphase();

		void FindPotentialCollisionPairs(std::vector<std::shared_ptr<PhysicsObject3D>> objects, std::vector<CollisionPair> &collisionPairs) override;
		void DebugDraw() override;
	};
}
