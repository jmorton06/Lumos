#pragma once

#include "Constraint.h"

namespace jm
{
	namespace maths
	{
		class Vector3;
	}


	class Quaternion;
	class PhysicsObject3D;

	class JM_EXPORT WeldConstraint : public Constraint
	{
	public:
		WeldConstraint(PhysicsObject3D *obj1, PhysicsObject3D *obj2);

		virtual void ApplyImpulse() override;
		virtual void DebugDraw() const override;

	protected:
		PhysicsObject3D *m_pObj1;
		PhysicsObject3D *m_pObj2;

		maths::Vector3	  m_positionOffset;
		maths::Quaternion m_orientation;
	};
}