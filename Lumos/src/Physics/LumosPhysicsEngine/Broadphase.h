#pragma once


#include "RigidBody3D.h"

namespace Lumos
{

	struct LUMOS_EXPORT CollisionPair
	{
		RigidBody3D* pObjectA;
		RigidBody3D* pObjectB;
	};

	class LUMOS_EXPORT Broadphase
	{
	public:
		virtual ~Broadphase() = default;
		virtual void FindPotentialCollisionPairs(std::vector<Ref<RigidBody3D>>& objects, std::vector<CollisionPair>& collisionPairs) = 0;
		virtual void DebugDraw() = 0;
	};
}
