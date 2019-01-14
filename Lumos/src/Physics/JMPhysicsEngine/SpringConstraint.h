#pragma once
#include "LM.h"
#include "Constraint.h"

namespace Lumos
{
	class PhysicsObject3D;

	class LUMOS_EXPORT SpringConstraint : public Constraint
	{
	public:
		SpringConstraint(PhysicsObject3D *obj1, PhysicsObject3D *obj2, const maths::Vector3 &globalOnA, const maths::Vector3 &globalOnB,
			float springConstant, float dampingFactor);

		virtual void ApplyImpulse() override;
		virtual void DebugDraw() const override;

	protected:
		PhysicsObject3D *m_pObj1;
		PhysicsObject3D *m_pObj2;

		float m_restDistance;

		float m_springConstant;
		float m_dampingFactor;

		maths::Vector3 m_LocalOnA;
		maths::Vector3 m_LocalOnB;
	};
}