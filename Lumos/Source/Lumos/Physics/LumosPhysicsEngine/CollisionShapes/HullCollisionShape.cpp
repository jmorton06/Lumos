#include "Precompiled.h"
#include "HullCollisionShape.h"
#include "Physics/LumosPhysicsEngine/RigidBody3D.h"
#include <glm/mat3x3.hpp>

#include "Graphics/Mesh.h"

namespace Lumos
{
    HullCollisionShape::HullCollisionShape()
    {
        m_HalfDimensions = glm::vec3(0.5f, 0.5f, 0.5f);
        m_Type           = CollisionShapeType::CollisionHull;
        m_Axes.resize(3);
    }

    HullCollisionShape::~HullCollisionShape()
    {
    }

    void HullCollisionShape::BuildFromMesh(Graphics::Mesh* mesh)
    {
        m_Hull = CreateSharedPtr<Hull>();

        auto vertexBuffer          = mesh->GetVertexBuffer();
        Graphics::Vertex* vertices = vertexBuffer->GetPointer<Graphics::Vertex>();
        uint32_t size              = vertexBuffer->GetSize();
        uint32_t count             = size / sizeof(Graphics::Vertex);

        uint32_t* indices   = mesh->GetIndexBuffer()->GetPointer<uint32_t>();
        uint32_t indexCount = mesh->GetIndexBuffer()->GetCount();

        for(size_t i = 0; i < count; i++)
        {
            m_Hull->AddVertex(vertices[i].Position);
        }

        for(size_t i = 0; i < indexCount; i += 3)
        {
            glm::vec3 n1     = vertices[indices[i]].Normal;
            glm::vec3 n2     = vertices[indices[i + 1]].Normal;
            glm::vec3 n3     = vertices[indices[i + 2]].Normal;
            glm::vec3 normal = n1 + n2 + n3;
            normal           = glm::normalize(normal);

            int vertexIdx[] = { (int)indices[i], (int)indices[i + 1], (int)indices[i + 2] };
            m_Hull->AddFace(normal, 3, vertexIdx);
        }

        m_Edges.resize(m_Hull->GetNumEdges());

        vertexBuffer->ReleasePointer();
        mesh->GetIndexBuffer()->ReleasePointer();
    }

    // glm::mat3 HullCollisionShape::GetLocalInertiaTensor(float mass)
    // {
    //     glm::mat3 inertia;
    //     float volume = m_HalfDimensions.x * m_HalfDimensions.y * m_HalfDimensions.z;
    //     float mass_per_volume = mass / volume;
    //     float diag_x = mass_per_volume * (m_HalfDimensions.y * m_HalfDimensions.y + m_HalfDimensions.z * m_HalfDimensions.z);
    //     float diag_y = mass_per_volume * (m_HalfDimensions.x * m_HalfDimensions.x + m_HalfDimensions.z * m_HalfDimensions.z);
    //     float diag_z = mass_per_volume * (m_HalfDimensions.x * m_HalfDimensions.x + m_HalfDimensions.y * m_HalfDimensions.y);
    //     inertia[0][0] = 1.0f / diag_x;
    //     inertia[1][1] = 1.0f / diag_y;
    //     inertia[2][2] = 1.0f / diag_z;
    //     return inertia;
    // }

    glm::mat3 HullCollisionShape::BuildInverseInertia(float invMass) const
    {
        LUMOS_PROFILE_FUNCTION();
        glm::mat3 inertia(1.0f);

        glm::vec3 dimsSq = (m_HalfDimensions + m_HalfDimensions);
        dimsSq           = dimsSq * dimsSq;

        inertia[0][0] = 12.f * invMass * 1.f / (dimsSq.y + dimsSq.z);
        inertia[1][1] = 12.f * invMass * 1.f / (dimsSq.x + dimsSq.z);
        inertia[2][2] = 12.f * invMass * 1.f / (dimsSq.x + dimsSq.y);

        return inertia;
    }

    std::vector<glm::vec3>& HullCollisionShape::GetCollisionAxes(const RigidBody3D* currentObject)
    {
        LUMOS_PROFILE_FUNCTION();
        {
            glm::mat3 objOrientation = glm::toMat3(currentObject->GetOrientation());
            m_Axes[0]                = (objOrientation * glm::vec3(1.0f, 0.0f, 0.0f)); // X - Axis
            m_Axes[1]                = (objOrientation * glm::vec3(0.0f, 1.0f, 0.0f)); // Y - Axis
            m_Axes[2]                = (objOrientation * glm::vec3(0.0f, 0.0f, 1.0f)); // Z - Axis
        }

        return m_Axes;
    }

    std::vector<CollisionEdge>& HullCollisionShape::GetEdges(const RigidBody3D* currentObject)
    {
        LUMOS_PROFILE_FUNCTION();
        {
            glm::mat4 transform = currentObject->GetWorldSpaceTransform() * m_LocalTransform;
            for(unsigned int i = 0; i < m_Hull->GetNumEdges(); ++i)
            {
                const HullEdge& edge = m_Hull->GetEdge(i);
                glm::vec3 A          = transform * glm::vec4(m_Hull->GetVertex(edge.vStart).pos, 1.0f);
                glm::vec3 B          = transform * glm::vec4(m_Hull->GetVertex(edge.vEnd).pos, 1.0f);

                m_Edges[i] = { A, B };
            }
        }
        return m_Edges;
    }

    void HullCollisionShape::GetMinMaxVertexOnAxis(const RigidBody3D* currentObject, const glm::vec3& axis, glm::vec3* out_min, glm::vec3* out_max) const
    {
        LUMOS_PROFILE_FUNCTION();
        glm::mat4 wsTransform      = glm::transpose(currentObject ? currentObject->GetWorldSpaceTransform() * m_LocalTransform : m_LocalTransform);
        const glm::vec3 local_axis = wsTransform * glm::vec4(axis, 1.0f);

        int vMin, vMax;

        m_Hull->GetMinMaxVerticesInAxis(local_axis, &vMin, &vMax);

        if(out_min)
            *out_min = wsTransform * glm::vec4(m_Hull->GetVertex(vMin).pos, 1.0f);
        if(out_max)
            *out_max = wsTransform * glm::vec4(m_Hull->GetVertex(vMax).pos, 1.0f);
    }

    void HullCollisionShape::GetIncidentReferencePolygon(const RigidBody3D* currentObject,
                                                         const glm::vec3& axis,
                                                         ReferencePolygon& refPolygon) const
    {
        LUMOS_PROFILE_FUNCTION();
        glm::mat4 wsTransform = currentObject ? currentObject->GetWorldSpaceTransform() * m_LocalTransform : m_LocalTransform;

        const glm::mat3 invNormalMatrix = glm::inverse(wsTransform);
        const glm::mat3 normalMatrix    = glm::transpose(invNormalMatrix);

        const glm::vec3 local_axis = invNormalMatrix * axis;

        int minVertex, maxVertex;
        m_Hull->GetMinMaxVerticesInAxis(local_axis, &minVertex, &maxVertex);

        const HullVertex& vert = m_Hull->GetVertex(maxVertex);

        const HullFace* best_face = nullptr;
        float best_correlation    = -FLT_MAX;
        for(int faceIdx : vert.enclosing_faces)
        {
            const HullFace* face         = &m_Hull->GetFace(faceIdx);
            const float temp_correlation = glm::dot(local_axis, face->normal);
            if(temp_correlation > best_correlation)
            {
                best_correlation = temp_correlation;
                best_face        = face;
            }
        }

        {
            if(best_face)
                refPolygon.Normal = normalMatrix * best_face->normal;
            refPolygon.Normal = glm::normalize(refPolygon.Normal);
        }

        if(best_face)
        {
            for(int vertIdx : best_face->vert_ids)
            {
                const HullVertex& vertex                 = m_Hull->GetVertex(vertIdx);
                refPolygon.Faces[refPolygon.FaceCount++] = wsTransform * glm::vec4(vertex.pos, 1.0f);
            }
        }

        if(best_face)
        {
            // Add the reference face itself to the list of adjacent planes
            glm::vec3 wsPointOnPlane = wsTransform * glm::vec4(m_Hull->GetVertex(m_Hull->GetEdge(best_face->edge_ids[0]).vStart).pos, 1.0f);
            glm::vec3 planeNrml      = -(normalMatrix * best_face->normal);
            planeNrml                = glm::normalize(planeNrml);
            float planeDist          = -glm::dot(planeNrml, wsPointOnPlane);

            refPolygon.AdjacentPlanes[refPolygon.PlaneCount++] = { planeNrml, planeDist };

            for(int edgeIdx : best_face->edge_ids)
            {
                const HullEdge& edge = m_Hull->GetEdge(edgeIdx);

                wsPointOnPlane = wsTransform * glm::vec4(m_Hull->GetVertex(edge.vStart).pos, 1.0f);

                for(int adjFaceIdx : edge.enclosing_faces)
                {
                    if(adjFaceIdx != best_face->idx)
                    {
                        const HullFace& adjFace = m_Hull->GetFace(adjFaceIdx);

                        planeNrml = -(normalMatrix * adjFace.normal);
                        planeNrml = glm::normalize(planeNrml);
                        planeDist = -glm::dot(planeNrml, wsPointOnPlane);

                        refPolygon.AdjacentPlanes[refPolygon.PlaneCount++] = { planeNrml, planeDist };
                    }
                }
            }
        }
    }

    void HullCollisionShape::DebugDraw(const RigidBody3D* currentObject) const
    {
        glm::mat4 transform = currentObject->GetWorldSpaceTransform() * m_LocalTransform;
        m_Hull->DebugDraw(transform);
    }
}
