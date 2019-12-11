#pragma once
#include "Constraint.h"

namespace Lumos
{
	namespace Maths
	{
		class Vector3;
	}

	class PhysicsObject3D;

	class LUMOS_EXPORT DistanceConstraint : public Constraint
	{
	public:
		DistanceConstraint(PhysicsObject3D *obj1, PhysicsObject3D *obj2, const Maths::Vector3 &globalOnA, const Maths::Vector3 &globalOnB);

		virtual void ApplyImpulse() override;
		virtual void DebugDraw() const override;

	protected:
		PhysicsObject3D *m_pObj1;
		PhysicsObject3D *m_pObj2;

		float m_Distance;

		Maths::Vector3 m_LocalOnA;
		Maths::Vector3 m_LocalOnB;
	};
}