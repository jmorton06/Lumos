#include "lmpch.h"
#include "PhysicsObject3D.h"
#include "LumosPhysicsEngine.h"

namespace Lumos
{

	PhysicsObject3D::PhysicsObject3D() : PhysicsObject()
		, m_wsTransformInvalidated(true)
		, m_RestVelocityThresholdSquared(0.001f)
		, m_AverageSummedVelocity(0.0f)
		, m_wsAabbInvalidated(true)
		, m_Position(0.0f, 0.0f, 0.0f)
		, m_LinearVelocity(0.0f, 0.0f, 0.0f)
		, m_Force(0.0f, 0.0f, 0.0f)
		, m_InvMass(0.0f)
		, m_Orientation(0.0f, 0.0f, 0.0f, 1.0f)
		, m_AngularVelocity(0.0f, 0.0f, 0.0f)
		, m_Torque(0.0f, 0.0f, 0.0f)
		, m_InvInertia(Maths::Matrix3::ZERO)
		, m_OnCollisionCallback(nullptr)
	{
		m_localBoundingBox.Define(-Maths::Vector3(0.5f, 0.5f, 0.5f), Maths::Vector3(0.5f, 0.5f, 0.5f));
	}

	PhysicsObject3D::~PhysicsObject3D()
	{
	}

	Maths::BoundingBox PhysicsObject3D::GetWorldSpaceAABB() const
	{
		if (m_wsAabbInvalidated)
		{
			const auto wm = GetWorldSpaceTransform();
			//m_localBoundingBox.Transform(wm);
			m_wsAabbInvalidated = false;
		}

		return m_wsAabb;
	}

	void PhysicsObject3D::WakeUp()
	{
		SetIsAtRest(false);
	}

	void PhysicsObject3D::SetIsAtRest(const bool isAtRest)
	{
		m_AtRest = isAtRest;
	}

	const Maths::Matrix4& PhysicsObject3D::GetWorldSpaceTransform() const
	{
		if (m_wsTransformInvalidated)
		{
			m_wsTransform = m_Orientation.RotationMatrix();
			m_wsTransform.SetTranslation(m_Position);

			m_wsTransformInvalidated = false;
		}

		return m_wsTransform;
	}

	void PhysicsObject3D::AutoResizeBoundingBox()
	{
		m_localBoundingBox.Clear();

		const Maths::Vector3 xAxis(1.0f, 0.0f, 0.0f);
		const Maths::Vector3 yAxis(0.0f, 1.0f, 0.0f);
		const Maths::Vector3 zAxis(0.0f, 0.0f, 1.0f);

		Maths::Vector3 lower, upper;

		if(m_CollisionShape)
		{
			m_CollisionShape->GetMinMaxVertexOnAxis(nullptr, xAxis, &lower, &upper);
			m_localBoundingBox.Merge(lower);
			m_localBoundingBox.Merge(upper);

			m_CollisionShape->GetMinMaxVertexOnAxis(nullptr, yAxis, &lower, &upper);
			m_localBoundingBox.Merge(lower);
			m_localBoundingBox.Merge(upper);

			m_CollisionShape->GetMinMaxVertexOnAxis(nullptr, zAxis, &lower, &upper);
			m_localBoundingBox.Merge(lower);
			m_localBoundingBox.Merge(upper);
		}

		m_wsAabbInvalidated = true;
	}

	void PhysicsObject3D::RestTest()
	{
		// Negative threshold disables test, don't bother calculating average or performing test
		if (m_RestVelocityThresholdSquared <= 0.0f)
			return;

		// Value between 0 and 1, higher values discard old data faster
		static const float ALPHA = 0.7f;

		// Calculate exponential moving average
		const float v = m_LinearVelocity.LengthSquared() + m_AngularVelocity.LengthSquared();
		m_AverageSummedVelocity += ALPHA * (v - m_AverageSummedVelocity);

		// Do test
		SetIsAtRest(m_AverageSummedVelocity <= m_RestVelocityThresholdSquared);
	}

	void PhysicsObject3D::DebugDraw(uint64_t flags) const
	{
	}

	nlohmann::json PhysicsObject3D::Serialise()
	{
		nlohmann::json output;
		output["typeID"] = LUMOS_TYPENAME(PhysicsObject3D);

		//output["collisionShape"]	= m_CollisionShape;

		return output;
	}

	void PhysicsObject3D::Deserialise(nlohmann::json& data)
	{

		//m_CollisionShape.Deserialise(data["collisionShape"]);
	}
}
