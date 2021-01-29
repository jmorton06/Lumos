#pragma once


#include "Broadphase.h"
#include "Maths/Maths.h"

namespace Lumos
{

	class LUMOS_EXPORT SortAndSweepBroadphase : public Broadphase
	{
	public:
		explicit SortAndSweepBroadphase(const Maths::Vector3& axis = Maths::Vector3(1.0f, 0.0f, 0.0f));
		virtual ~SortAndSweepBroadphase();

		inline Maths::Vector3 Axis() const
		{
			return m_axis;
		}

		void SetAxis(const Maths::Vector3& axis);

		void FindPotentialCollisionPairs(Ref<RigidBody3D>* objects, uint32_t objectCount, std::vector<CollisionPair>& collisionPairs) override;
		void DebugDraw() override;

	protected:
		Maths::Vector3 m_axis; //Axis along which testing is performed
		int m_axisIndex; //Index of axis along which testing is performed
	};
}
