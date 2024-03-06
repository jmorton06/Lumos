#include "Precompiled.h"
#include "RigidBody3D.h"
#include "LumosPhysicsEngine.h"
#include "Graphics/Renderers/DebugRenderer.h"

#include "Physics/LumosPhysicsEngine/CollisionShapes/SphereCollisionShape.h"
#include "Physics/LumosPhysicsEngine/CollisionShapes/CuboidCollisionShape.h"
#include "Physics/LumosPhysicsEngine/CollisionShapes/PyramidCollisionShape.h"
#include "Physics/LumosPhysicsEngine/CollisionShapes/HullCollisionShape.h"
#include "Physics/LumosPhysicsEngine/CollisionShapes/CapsuleCollisionShape.h"

namespace Lumos
{

    RigidBody3D::RigidBody3D(const RigidBody3DProperties& properties)
        : m_WSTransformInvalidated(true)
        , m_RestVelocityThresholdSquared(0.004f)
        , m_AverageSummedVelocity(0.0f)
        , m_WSAabbInvalidated(true)
        , m_Position(properties.Position)
        , m_LinearVelocity(properties.LinearVelocity)
        , m_Force(properties.Force)
        , m_Orientation(properties.Orientation)
        , m_AngularVelocity(properties.AngularVelocity)
        , m_Torque(properties.Torque)
        , m_InvInertia(glm::mat3(1.0f))
        , m_OnCollisionCallback(nullptr)
        , m_AngularFactor(1.0f)
        , m_WSTransform(glm::mat4(1.0f))
    {
        LUMOS_ASSERT(properties.Mass > 0.0f, "Mass <= 0");
        m_InvMass = 1.0f / properties.Mass;

        m_LocalBoundingBox.Set(glm::vec3(-0.5f), glm::vec3(0.5f));

        if(properties.Shape)
            SetCollisionShape(properties.Shape);

        m_Static     = properties.Static;
        m_AtRest     = properties.AtRest;
        m_Elasticity = properties.Elasticity;
        m_Friction   = properties.Friction;
        m_UUID       = UUID();
    }

    RigidBody3D::~RigidBody3D()
    {
        m_UUID = 0;
    }

    const Maths::BoundingBox& RigidBody3D::GetWorldSpaceAABB()
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        if(m_WSAabbInvalidated)
        {
            LUMOS_PROFILE_SCOPE_LOW("Calculate BoundingBox");
            m_WSAabb            = m_LocalBoundingBox.Transformed(GetWorldSpaceTransform());
            m_WSAabbInvalidated = false;
        }

        return m_WSAabb;
    }

    void RigidBody3D::WakeUp()
    {
        SetIsAtRest(false);
    }

    void RigidBody3D::SetIsAtRest(const bool isAtRest)
    {
        m_AtRest = isAtRest;
    }

    const glm::mat4& RigidBody3D::GetWorldSpaceTransform() const
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        if(m_WSTransformInvalidated)
        {
            m_WSTransform = glm::translate(glm::mat4(1.0), m_Position) * glm::toMat4(m_Orientation);

            m_WSTransformInvalidated = false;
        }

        return m_WSTransform;
    }

    void RigidBody3D::AutoResizeBoundingBox()
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        m_LocalBoundingBox.Clear();

        const glm::vec3 xAxis(1.0f, 0.0f, 0.0f);
        const glm::vec3 yAxis(0.0f, 1.0f, 0.0f);
        const glm::vec3 zAxis(0.0f, 0.0f, 1.0f);

        glm::vec3 lower, upper;

        if(m_CollisionShape)
        {
            m_CollisionShape->GetMinMaxVertexOnAxis(nullptr, xAxis, &lower, &upper);
            m_LocalBoundingBox.Merge(lower);
            m_LocalBoundingBox.Merge(upper);

            m_CollisionShape->GetMinMaxVertexOnAxis(nullptr, yAxis, &lower, &upper);
            m_LocalBoundingBox.Merge(lower);
            m_LocalBoundingBox.Merge(upper);

            m_CollisionShape->GetMinMaxVertexOnAxis(nullptr, zAxis, &lower, &upper);
            m_LocalBoundingBox.Merge(lower);
            m_LocalBoundingBox.Merge(upper);
        }

        m_WSAabbInvalidated = true;
    }

    void RigidBody3D::RestTest()
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        // Negative threshold disables test, don't bother calculating average or performing test
        if(m_RestVelocityThresholdSquared <= 0.0f)
            return;

        // Value between 0 and 1, higher values discard old data faster
        static const float ALPHA = 0.15f;

        // Calculate exponential moving average
        const float v = glm::length2(m_LinearVelocity) + glm::length2(m_AngularVelocity);
        m_AverageSummedVelocity += ALPHA * (v - m_AverageSummedVelocity);

        // Do test
        SetIsAtRest(m_AverageSummedVelocity <= m_RestVelocityThresholdSquared);
    }

    void RigidBody3D::DebugDraw(uint64_t flags) const
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        glm::vec4 colour(0.4f, 0.3f, 0.7f, 1.0f);

        if(flags & PhysicsDebugFlags::AABB)
        {
            if(!IsAwake())
                colour = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);

            // AABB
            Maths::BoundingBox box = m_WSAabb;
            DebugRenderer::DebugDraw(box, colour, false);
        }

        if(flags & PhysicsDebugFlags::LINEARVELOCITY)
            DebugRenderer::DrawThickLine(m_WSTransform[3], m_WSTransform * glm::vec4(m_LinearVelocity, 1.0f), 0.02f, false, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));

        if(flags & PhysicsDebugFlags::LINEARFORCE)
            DebugRenderer::DrawThickLine(m_WSTransform[3], m_WSTransform * glm::vec4(m_Force, 1.0f), 0.02f, false, glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
    }

    void RigidBody3D::SetCollisionShape(CollisionShapeType type)
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        switch(type)
        {
        case CollisionShapeType::CollisionCuboid:
            SetCollisionShape(CreateSharedPtr<CuboidCollisionShape>());
            break;
        case CollisionShapeType::CollisionSphere:
            SetCollisionShape(CreateSharedPtr<SphereCollisionShape>());
            break;
        case CollisionShapeType::CollisionPyramid:
            SetCollisionShape(CreateSharedPtr<PyramidCollisionShape>());
            break;
        case CollisionShapeType::CollisionCapsule:
            SetCollisionShape(CreateSharedPtr<CapsuleCollisionShape>());
            break;
        default:
            LUMOS_LOG_ERROR("Unsupported Collision shape");
            break;
        }
    }
}
