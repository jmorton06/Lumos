#include "LM.h"
#include "BruteForceBroadphase.h"

namespace Lumos
{

	BruteForceBroadphase::BruteForceBroadphase()
		: Broadphase()
	{
	}

	BruteForceBroadphase::~BruteForceBroadphase()
	{
	}

	void BruteForceBroadphase::FindPotentialCollisionPairs(std::vector<Ref<PhysicsObject3D>> objects,
	                                                       std::vector<CollisionPair> &collisionPairs)
	{
		if (objects.empty())
			return;

		for (size_t i = 0; i < objects.size() - 1; ++i)
		{
			for (size_t j = i + 1; j < objects.size(); ++j)
			{
				PhysicsObject3D* obj1 = objects[i].get();
				PhysicsObject3D* obj2 = objects[j].get();

				if (obj1->GetIsAtRest() && obj2->GetIsAtRest())
					continue;

				// Skip pairs of two at static objects
				if (obj1->GetIsStatic() && obj2->GetIsStatic())
					continue;

				// Skip pairs of one static and one at rest
				if (obj1->GetIsAtRest() && obj2->GetIsStatic())
					continue;

				if (obj1->GetIsStatic() && obj2->GetIsAtRest())
					continue;

				// Check they both have collision shapes and at least one is awake
				if (obj1->GetCollisionShape() && obj2->GetCollisionShape() && (obj1->IsAwake() || obj2->IsAwake()))
				{
					CollisionPair cp;
					cp.pObjectA = obj1;
					cp.pObjectB = obj2;

					collisionPairs.push_back(cp);
				}
			}
		}
	}

	void BruteForceBroadphase::DebugDraw()
	{
	}
}
