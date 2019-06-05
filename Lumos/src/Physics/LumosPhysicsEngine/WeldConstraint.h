#pragma once

#include "Constraint.h"

namespace Lumos
{
	namespace Maths
	{
		class Vector3;
	}


	class Quaternion;
	class PhysicsObject3D;

	class LUMOS_EXPORT WeldConstraint : public Constraint
	{
	public:
		WeldConstraint(PhysicsObject3D *obj1, PhysicsObject3D *obj2);

		virtual void ApplyImpulse() override;
		virtual void DebugDraw() const override;

	protected:
		PhysicsObject3D *m_pObj1;
		PhysicsObject3D *m_pObj2;

		Maths::Vector3	  m_positionOffset;
		Maths::Quaternion m_orientation;
	};
}