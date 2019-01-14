#pragma once

#include "JM.h"
#include "PhysicsObject3D.h"

namespace jm
{

	struct JM_EXPORT CollisionPair
	{
		PhysicsObject3D *pObjectA;
		PhysicsObject3D *pObjectB;
	};

	class JM_EXPORT Broadphase
	{
	public:
		virtual ~Broadphase() = default;
		virtual void FindPotentialCollisionPairs(std::vector<std::shared_ptr<PhysicsObject3D>> objects, std::vector<CollisionPair> &collisionPairs) = 0;
		virtual void DebugDraw() = 0;
	};
}