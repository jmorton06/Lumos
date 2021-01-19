#pragma once


#include "Broadphase.h"
#include "Maths/Maths.h"

namespace Lumos
{

	class LUMOS_EXPORT BruteForceBroadphase : public Broadphase
	{
	public:
		explicit BruteForceBroadphase(const Maths::Vector3& axis = Maths::Vector3(0.0f));
		virtual ~BruteForceBroadphase();

		void FindPotentialCollisionPairs(Ref<RigidBody3D>* objects, u32 objectCount, std::vector<CollisionPair>& collisionPairs) override;
		void DebugDraw() override;

	private:
		Maths::Vector3 m_axis;
	};
}
