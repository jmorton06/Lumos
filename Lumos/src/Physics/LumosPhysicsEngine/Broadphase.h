#pragma once

#include "LM.h"
#include "PhysicsObject3D.h"

namespace Lumos
{

	struct LUMOS_EXPORT CollisionPair
	{
		PhysicsObject3D *pObjectA;
		PhysicsObject3D *pObjectB;
	};

	class LUMOS_EXPORT Broadphase
	{
	public:
		virtual ~Broadphase() = default;
		virtual void FindPotentialCollisionPairs(std::vector<Ref<PhysicsObject3D>> objects, std::vector<CollisionPair> &collisionPairs) = 0;
		virtual void DebugDraw() = 0;
	};
}
