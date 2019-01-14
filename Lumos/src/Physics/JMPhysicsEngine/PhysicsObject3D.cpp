#include "LM.h"
#include "PhysicsObject3D.h"
#include "JMPhysicsEngine.h"
#include "Entity/Entity.h"

#include "Graphics/Renderers/DebugRenderer.h"

namespace Lumos
{

	PhysicsObject3D::PhysicsObject3D() : PhysicsObject()
		, m_pParent(nullptr)
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
		, m_InvInertia(maths::Matrix3::ZeroMatrix)
		, m_OnCollisionCallback(nullptr)
	{
		m_localBoundingBox.SetHalfDimensions(maths::Vector3(0.5f, 0.5f, 0.5f));
	}

	PhysicsObject3D::~PhysicsObject3D()
	{
	}

	maths::BoundingBox PhysicsObject3D::GetWorldSpaceAABB() const
	{
		if (m_wsAabbInvalidated)
		{
			m_wsAabb = m_localBoundingBox.Transform(GetWorldSpaceTransform());
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

	const maths::Matrix4& PhysicsObject3D::GetWorldSpaceTransform() const
	{
		if (m_wsTransformInvalidated)
		{
			m_wsTransform = m_Orientation.ToMatrix4();
			m_wsTransform.SetPositionVector(m_Position);

			m_wsTransformInvalidated = false;
		}

		return m_wsTransform;
	}

	void PhysicsObject3D::AutoResizeBoundingBox()
	{
		m_localBoundingBox.Reset();

		const maths::Vector3 xAxis(1.0f, 0.0f, 0.0f);
		const maths::Vector3 yAxis(0.0f, 1.0f, 0.0f);
		const maths::Vector3 zAxis(0.0f, 0.0f, 1.0f);

		maths::Vector3 lower, upper;

		if(m_CollisionShape)
		{
			m_CollisionShape->GetMinMaxVertexOnAxis(nullptr, xAxis, &lower, &upper);
			m_localBoundingBox.ExpandToFit(lower);
			m_localBoundingBox.ExpandToFit(upper);

			m_CollisionShape->GetMinMaxVertexOnAxis(nullptr, yAxis, &lower, &upper);
			m_localBoundingBox.ExpandToFit(lower);
			m_localBoundingBox.ExpandToFit(upper);

			m_CollisionShape->GetMinMaxVertexOnAxis(nullptr, zAxis, &lower, &upper);
			m_localBoundingBox.ExpandToFit(lower);
			m_localBoundingBox.ExpandToFit(upper);
		}
		
		// Set bounding radius of parent
		if (m_pParent != nullptr)
			m_pParent->SetBoundingRadius(m_localBoundingBox.SphereRadius() * 2.0f);

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
		maths::Vector4 colour(0.2f, 0.8f, 1.0f, 1.0f);

		if (flags & DEBUGDRAW_FLAGS_AABB)
		{
			if (!IsAwake())
				colour = maths::Vector4(0.0f, 1.0f, 0.0f, 1.0f);

			// AABB
			//GetWorldSpaceAABB().DebugDraw(Matrix4(), Vector4(0.8f, 1.0f, 1.0f, 0.25f), colour);
			maths::BoundingBox box = GetWorldSpaceAABB();
			DebugRenderer::DebugDraw(&box, colour);
		}

		// Object bounding radius
		if (m_pParent != nullptr && flags & DEBUGDRAW_FLAGS_BOUNDING_RADIUS)
		{
			maths::Vector4 boundRadiusCol(colour);
			boundRadiusCol.SetW(0.2f);
			DebugRenderer::DrawPointNDT(GetWorldSpaceTransform().GetPositionVector(), m_pParent->GetBoundingRadius(), boundRadiusCol);
		}

		if (flags & DEBUGDRAW_FLAGS_LINEARVELOCITY)
			DebugRenderer::DrawThickLineNDT(m_wsTransform.GetPositionVector(), m_wsTransform * m_LinearVelocity, 0.02f,
											maths::Vector4(0.0f, 1.0f, 0.0f, 1.0f));

		if (flags & DEBUGDRAW_FLAGS_LINEARFORCE)
			DebugRenderer::DrawThickLineNDT(m_wsTransform.GetPositionVector(), m_wsTransform * m_Force, 0.02f,
											maths::Vector4(0.0f, 0.0f, 1.0f, 1.0f));
	}
}
