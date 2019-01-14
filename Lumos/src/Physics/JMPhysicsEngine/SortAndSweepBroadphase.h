#pragma once

#include "LM.h"
#include "Broadphase.h"
#include "Maths/Maths.h"

namespace Lumos
{

	class LUMOS_EXPORT SortAndSweepBroadphase : public Broadphase
	{
	public:
		explicit SortAndSweepBroadphase(const maths::Vector3 &axis = maths::Vector3(1.0f, 0.0f, 0.0f));
		virtual ~SortAndSweepBroadphase();

		inline maths::Vector3 Axis() const
		{
			return m_axis;
		}

		void SetAxis(const maths::Vector3 &axis);

		void FindPotentialCollisionPairs(std::vector<std::shared_ptr<PhysicsObject3D>> objects, std::vector<CollisionPair> &collisionPairs) override;
		void DebugDraw() override;

	protected:
		maths::Vector3 m_axis;  //Axis along which testing is performed
		int m_axisIndex; //Index of axis along which testing is performed
	};
}
