#include "JM.h"
#include "JMPhysicsEngine.h"

#include "SpringConstraint.h"
#include "Graphics/Renderers/DebugRenderer.h"

namespace jm
{

	SpringConstraint::SpringConstraint(PhysicsObject3D *obj1, PhysicsObject3D *obj2, const maths::Vector3 &globalOnA, const maths::Vector3 &globalOnB,
		float springConstant, float dampingFactor)
		: m_pObj1(obj1)
		, m_pObj2(obj2)
		, m_springConstant(springConstant)
		, m_dampingFactor(dampingFactor)
	{
		maths::Vector3 ab = globalOnB - globalOnA;
		m_restDistance = ab.Length();

		maths::Vector3 r1 = (globalOnA - m_pObj1->GetPosition());
		maths::Vector3 r2 = (globalOnB - m_pObj2->GetPosition());
		m_LocalOnA = maths::Matrix3::Transpose(m_pObj1->GetOrientation().ToMatrix3()) * r1;
		m_LocalOnB = maths::Matrix3::Transpose(m_pObj2->GetOrientation().ToMatrix3()) * r2;
	}

	void SpringConstraint::ApplyImpulse()
	{
		if (m_pObj1->GetInverseMass() + m_pObj2->GetInverseMass() == 0.0f)
			return;

		maths::Vector3 r1 = m_pObj1->GetOrientation().ToMatrix3() * m_LocalOnA;
		maths::Vector3 r2 = m_pObj2->GetOrientation().ToMatrix3() * m_LocalOnB;

		maths::Vector3 globalOnA = r1 + m_pObj1->GetPosition();
		maths::Vector3 globalOnB = r2 + m_pObj2->GetPosition();

		maths::Vector3 ab = globalOnB - globalOnA;
		maths::Vector3 abn = ab;
		abn.Normalise();

		maths::Vector3 v0 = m_pObj1->GetLinearVelocity() + maths::Vector3::Cross(m_pObj1->GetAngularVelocity(), r1);
		maths::Vector3 v1 = m_pObj2->GetLinearVelocity() + maths::Vector3::Cross(m_pObj2->GetAngularVelocity(), r2);
		float constraintMass = (m_pObj1->GetInverseMass() + m_pObj2->GetInverseMass()) +
						maths::Vector3::Dot(abn, maths::Vector3::Cross(m_pObj1->GetInverseInertia() * maths::Vector3::Cross(r1, abn), r1) +
						maths::Vector3::Cross(m_pObj2->GetInverseInertia() * maths::Vector3::Cross(r2, abn), r2));

		float b = 0.0f;
		{
			float distanceOffset = ab.Length() - m_restDistance;
			float baumgarteScalar = 0.1f;
			b = -(baumgarteScalar / JMPhysicsEngine::Instance()->GetDeltaTime()) * distanceOffset;
		}

		float jn = (-(maths::Vector3::Dot(v0 - v1, abn) + b) * m_springConstant) - (m_dampingFactor * (v0 - v1).Length());
		jn /= constraintMass;

		m_pObj1->SetLinearVelocity(m_pObj1->GetLinearVelocity() + abn * (jn * m_pObj1->GetInverseMass()));
		m_pObj2->SetLinearVelocity(m_pObj2->GetLinearVelocity() - abn * (jn * m_pObj2->GetInverseMass()));

		m_pObj1->SetAngularVelocity(m_pObj1->GetAngularVelocity() + m_pObj1->GetInverseInertia() * maths::Vector3::Cross(r1, abn * jn));
		m_pObj2->SetAngularVelocity(m_pObj2->GetAngularVelocity() - m_pObj2->GetInverseInertia() * maths::Vector3::Cross(r2, abn * jn));
	}

	void SpringConstraint::DebugDraw() const
	{
		maths::Vector3 globalOnA = m_pObj1->GetOrientation().ToMatrix3() * m_LocalOnA + m_pObj1->GetPosition();
		maths::Vector3 globalOnB = m_pObj2->GetOrientation().ToMatrix3() * m_LocalOnB + m_pObj2->GetPosition();

		DebugRenderer::DrawThickLine(globalOnA, globalOnB, 0.02f, maths::Vector4(0.0f, 0.0f, 0.0f, 1.0f));
		DebugRenderer::DrawPointNDT(globalOnA, 0.05f, maths::Vector4(1.0f, 0.8f, 1.0f, 1.0f));
		DebugRenderer::DrawPointNDT(globalOnB, 0.05f, maths::Vector4(1.0f, 0.8f, 1.0f, 1.0f));
	}
}