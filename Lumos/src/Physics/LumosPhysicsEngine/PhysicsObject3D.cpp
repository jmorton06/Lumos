#include "lmpch.h"
#include "PhysicsObject3D.h"
#include "LumosPhysicsEngine.h"
#include "Graphics/Renderers/DebugRenderer.h"

#include "CuboidCollisionShape.h"
#include "PyramidCollisionShape.h"
#include "SphereCollisionShape.h"
#include "CapsuleCollisionShape.h"

namespace Lumos
{

	PhysicsObject3D::PhysicsObject3D(const Physics3DProperties& properties)
		: m_wsTransformInvalidated(true)
		, m_RestVelocityThresholdSquared(0.001f)
		, m_AverageSummedVelocity(0.0f)
		, m_wsAabbInvalidated(true)
		, m_Position(properties.Position)
		, m_LinearVelocity(properties.LinearVelocity)
		, m_Force(properties.Force)
		, m_Orientation(properties.Orientation)
		, m_AngularVelocity(properties.AngularVelocity)
		, m_Torque(properties.Torque)
		, m_InvInertia(Maths::Matrix3::ZERO)
		, m_OnCollisionCallback(nullptr)
	{
        LUMOS_ASSERT(properties.Mass > 0.0f, "Mass <= 0");
        m_InvMass = 1.0f / properties.Mass;
    
        m_localBoundingBox.Define(Maths::Vector3(-0.5f), Maths::Vector3(0.5f));

        if(properties.Shape)
            SetCollisionShape(properties.Shape);
    
        m_Static = properties.Static;
        m_AtRest = properties.AtRest;
        m_Elasticity = properties.Elasticity;
        m_Friction = properties.Friction;
	}

	PhysicsObject3D::~PhysicsObject3D()
	{
	}

	Maths::BoundingBox PhysicsObject3D::GetWorldSpaceAABB()
	{
		if (m_wsAabbInvalidated)
		{
			m_wsAabb = m_localBoundingBox.Transformed(GetWorldSpaceTransform());
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
			m_wsTransform = m_Orientation.RotationMatrix4();
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
        Maths::Vector4 colour(0.4f, 0.3f, 0.7f, 1.0f);

        if (flags & PhysicsDebugFlags::AABB)
        {
            if (!IsAwake())
                colour = Maths::Vector4(0.0f, 1.0f, 0.0f, 1.0f);

            // AABB
            Maths::BoundingBox box = m_wsAabb;
            DebugRenderer::DebugDraw(box, colour, false);
        }

        if (flags & PhysicsDebugFlags::LINEARVELOCITY)
            DebugRenderer::DrawThickLineNDT(m_wsTransform.Translation(), m_wsTransform * m_LinearVelocity, 0.02f,
                                           Maths::Vector4(0.0f, 1.0f, 0.0f, 1.0f));

        if (flags & PhysicsDebugFlags::LINEARFORCE)
            DebugRenderer::DrawThickLineNDT(m_wsTransform.Translation(), m_wsTransform * m_Force, 0.02f,
                                            Maths::Vector4(0.0f, 0.0f, 1.0f, 1.0f));
	}
    
        void PhysicsObject3D::SetCollisionShape(CollisionShapeType type)
        {
            switch (type)
            {
                case CollisionShapeType::CollisionCuboid:
                    SetCollisionShape(new CuboidCollisionShape());
                    break;
                case CollisionShapeType::CollisionSphere:
                    SetCollisionShape(new SphereCollisionShape());
                    break;
                case CollisionShapeType::CollisionPyramid:
                    SetCollisionShape(new PyramidCollisionShape());
                    break;
                case CollisionShapeType::CollisionCapsule:
                    SetCollisionShape(new CapsuleCollisionShape());
                    break;
                default:
                    Lumos::Debug::Log::Error("Unsupported Collision shape");
                    break;
            }
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
