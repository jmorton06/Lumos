#pragma once

#include "LM.h"
#include "Constraint.h"

namespace Lumos
{
	namespace maths
	{
		class Vector3;
	}

	class PhysicsObject3D;

	class LUMOS_EXPORT DistanceConstraint : public Constraint
	{
	public:
		DistanceConstraint(PhysicsObject3D *obj1, PhysicsObject3D *obj2, const maths::Vector3 &globalOnA, const maths::Vector3 &globalOnB);

		virtual void ApplyImpulse() override;
		virtual void DebugDraw() const override;

	protected:
		PhysicsObject3D *m_pObj1;
		PhysicsObject3D *m_pObj2;

		float m_Distance;

		maths::Vector3 m_LocalOnA;
		maths::Vector3 m_LocalOnB;
	};
}