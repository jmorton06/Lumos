#include "Precompiled.h"
#include "SphereCollisionShape.h"
#include "Physics/LumosPhysicsEngine/RigidBody3D.h"
#include <glm/mat3x3.hpp>
#include "Graphics/Renderers/DebugRenderer.h"

namespace Lumos
{

    SphereCollisionShape::SphereCollisionShape()
    {
        m_Radius = 1.0f;
        m_LocalTransform = glm::scale(glm::mat4(1.0), glm::vec3(m_Radius * 2.0f));
        m_Type = CollisionShapeType::CollisionSphere;
    }

    SphereCollisionShape::SphereCollisionShape(float radius)
    {
        m_Radius = radius;
        m_LocalTransform = glm::scale(glm::mat4(1.0), glm::vec3(m_Radius * 2.0f));
        m_Type = CollisionShapeType::CollisionSphere;
    }

    SphereCollisionShape::~SphereCollisionShape()
    {
    }

    glm::mat3 SphereCollisionShape::BuildInverseInertia(float invMass) const
    {
        LUMOS_PROFILE_FUNCTION();
        float i = 2.5f * invMass / (m_Radius * m_Radius); // SOLID
        // float i = 1.5f * invMass * m_Radius * m_Radius; //HOLLOW

        glm::mat3 inertia;
        inertia[0][0] = i;
        inertia[1][1] = i;
        inertia[2][2] = i;

        return inertia;
    }

    std::vector<glm::vec3>& SphereCollisionShape::GetCollisionAxes(const RigidBody3D* currentObject)
    {
        /* There is infinite edges so handle seperately */
        m_Axes.clear();
        return m_Axes;
    }

    std::vector<CollisionEdge>& SphereCollisionShape::GetEdges(const RigidBody3D* currentObject)
    {
        /* There is infinite edges on a sphere so handle seperately */
        return m_Edges;
    }

    void SphereCollisionShape::GetMinMaxVertexOnAxis(const RigidBody3D* currentObject, const glm::vec3& axis, glm::vec3* out_min, glm::vec3* out_max) const
    {
        LUMOS_PROFILE_FUNCTION();
        glm::mat4 transform = currentObject ? currentObject->GetWorldSpaceTransform() * m_LocalTransform : m_LocalTransform;

        glm::vec3 pos = transform[3];

        if(out_min)
            *out_min = pos - axis * m_Radius;

        if(out_max)
            *out_max = pos + axis * m_Radius;
    }

    void SphereCollisionShape::GetIncidentReferencePolygon(const RigidBody3D* currentObject,
        const glm::vec3& axis,
        ReferencePolygon& refPolygon) const
    {
        LUMOS_PROFILE_FUNCTION();
        refPolygon.Faces[0] = currentObject->GetPosition() + axis * m_Radius;
        refPolygon.FaceCount = 1;

        refPolygon.Normal = axis;
    }

    void SphereCollisionShape::DebugDraw(const RigidBody3D* currentObject) const
    {
        LUMOS_PROFILE_FUNCTION();
        glm::mat4 transform = currentObject->GetWorldSpaceTransform() * m_LocalTransform;

        auto pos = transform[3];
        auto sphere = Maths::BoundingSphere(pos, m_Radius);
        DebugRenderer::DebugDraw(sphere, glm::vec4(1.0f, 1.0f, 1.0f, 0.2f));
        DebugRenderer::DebugDrawSphere(m_Radius, pos, glm::vec4(1.0f, 0.3f, 1.0f, 1.0f));
    }
}
