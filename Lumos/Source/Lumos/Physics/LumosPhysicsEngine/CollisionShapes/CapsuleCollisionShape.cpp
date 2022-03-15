#include "Precompiled.h"
#include "CapsuleCollisionShape.h"
#include "Physics/LumosPhysicsEngine/RigidBody3D.h"
#include "Graphics/Renderers/DebugRenderer.h"
#include <glm/mat3x3.hpp>
#include <glm/gtx/matrix_decompose.hpp>

namespace Lumos
{
    CapsuleCollisionShape::CapsuleCollisionShape(float radius, float height)
    {
        m_Radius = radius;
        m_Height = height;
        m_LocalTransform = glm::mat4(1.0);//glm::scale(glm::mat4(1.0), glm::vec3(m_Radius));
        m_Type = CollisionShapeType::CollisionCapsule;
    }

    CapsuleCollisionShape::~CapsuleCollisionShape()
    {
    }

    glm::mat3 CapsuleCollisionShape::BuildInverseInertia(float invMass) const
    {
        glm::vec3 halfExtents(m_Radius, m_Radius, m_Radius);
        halfExtents.x += m_Height * 0.5f;

        float lx = 2.0f * (halfExtents.x);
        float ly = 2.0f * (halfExtents.y);
        float lz = 2.0f * (halfExtents.z);
        const float x2 = lx * lx;
        const float y2 = ly * ly;
        const float z2 = lz * lz;
        const float scaledmass = (1.0f / invMass) * float(.08333333);

        glm::mat3 inertia(1.0f);

        inertia[0][0] = 1.0f / scaledmass * (y2 + z2);
        inertia[1][1] = 1.0f / scaledmass * (x2 + z2);
        inertia[2][2] = 1.0f / scaledmass * (x2 + y2);

        return inertia;
    }

    std::vector<glm::vec3>& CapsuleCollisionShape::GetCollisionAxes(const RigidBody3D* currentObject)
    {
        /* There is infinite edges so handle seperately */
        return m_Axes;
    }

    std::vector<CollisionEdge>& CapsuleCollisionShape::GetEdges(const RigidBody3D* currentObject)
    {
        /* There is infinite edges on a sphere so handle seperately */
        return m_Edges;
    }

    void CapsuleCollisionShape::GetMinMaxVertexOnAxis(const RigidBody3D* currentObject, const glm::vec3& axis, glm::vec3* out_min, glm::vec3* out_max) const
    {
        glm::mat4 transform = currentObject ? currentObject->GetWorldSpaceTransform() * m_LocalTransform : m_LocalTransform;
        glm::vec3 pos = transform[3]; 

        if(out_min)
            *out_min = pos - axis * m_Radius;

        if(out_max)
            *out_max = pos + axis * m_Radius;
    }

    void CapsuleCollisionShape::GetIncidentReferencePolygon(const RigidBody3D* currentObject,
        const glm::vec3& axis,
        ReferencePolygon& refPolygon) const
    {
        refPolygon.Faces[0] = currentObject->GetPosition() + axis * m_Radius;
        refPolygon.FaceCount = 1;

        refPolygon.Normal = axis;
    }

    void CapsuleCollisionShape::DebugDraw(const RigidBody3D* currentObject) const
    {
        LUMOS_PROFILE_FUNCTION();
        glm::mat4 transform = currentObject->GetWorldSpaceTransform() * m_LocalTransform;

        glm::vec3 scale;
        glm::quat rotation;
        glm::vec3 translation;
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(transform, scale, rotation, translation, skew, perspective);
        DebugRenderer::DebugDrawCapsule(translation, rotation, m_Height, m_Radius, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
    }
}
