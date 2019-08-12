#pragma once

#include "LM.h"
#include "Broadphase.h"

namespace Lumos
{

	class LUMOS_EXPORT BruteForceBroadphase : public Broadphase
	{
	public:
		BruteForceBroadphase();
		virtual ~BruteForceBroadphase();

		void FindPotentialCollisionPairs(std::vector<Ref<PhysicsObject3D>> objects, std::vector<CollisionPair> &collisionPairs) override;
		void DebugDraw() override;
	};
}
