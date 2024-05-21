#include "Precompiled.h"
#include "CuboidCollisionShape.h"
#include "Physics/LumosPhysicsEngine/RigidBody3D.h"
#include <glm/ext/matrix_float3x3.hpp>

namespace Lumos
{
    CuboidCollisionShape::CuboidCollisionShape()
    {
        m_CuboidHalfDimensions = glm::vec3(0.5f, 0.5f, 0.5f);
        m_Type                 = CollisionShapeType::CollisionCuboid;
        m_LocalTransform       = glm::mat4(1.0f); // glm::scale(glm::mat4(1.0), m_CuboidHalfDimensions);
        m_CubeHull             = CreateSharedPtr<BoundingBoxHull>();
        m_CubeHull->Set(-m_CuboidHalfDimensions, m_CuboidHalfDimensions);
        m_CubeHull->UpdateHull();
        m_Axes.resize(3);
        m_Edges.resize(m_CubeHull->GetNumEdges());
    }

    CuboidCollisionShape::CuboidCollisionShape(const glm::vec3& halfdims)
    {
        m_CuboidHalfDimensions = halfdims;
        m_LocalTransform       = glm::mat4(1.0f); // glm::scale(glm::mat4(1.0), halfdims);
        m_Type                 = CollisionShapeType::CollisionCuboid;

        m_CubeHull = CreateSharedPtr<BoundingBoxHull>();
        m_CubeHull->Set(-m_CuboidHalfDimensions, m_CuboidHalfDimensions);
        m_CubeHull->UpdateHull();

        m_Axes.resize(3);
        m_Edges.resize(m_CubeHull->GetNumEdges());
    }

    CuboidCollisionShape::~CuboidCollisionShape()
    {
    }

    glm::mat3 CuboidCollisionShape::BuildInverseInertia(float invMass) const
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        // https://en.wikipedia.org/wiki/List_of_moments_of_inertia
        glm::mat3 inertia(1.0f);

        glm::vec3 dimsSq = (m_CuboidHalfDimensions + m_CuboidHalfDimensions);
        dimsSq           = dimsSq * dimsSq;

        inertia[0][0] = 12.f * invMass * 1.f / (dimsSq.y + dimsSq.z);
        inertia[1][1] = 12.f * invMass * 1.f / (dimsSq.x + dimsSq.z);
        inertia[2][2] = 12.f * invMass * 1.f / (dimsSq.x + dimsSq.y);

        return inertia;
    }

    std::vector<glm::vec3>& CuboidCollisionShape::GetCollisionAxes(const RigidBody3D* currentObject)
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        {
            m_Axes.resize(3);

            glm::mat3 objOrientation = glm::toMat3(currentObject->GetOrientation());   //.RotationMatrix();
            m_Axes[0]                = (objOrientation * glm::vec3(1.0f, 0.0f, 0.0f)); // X - Axis
            m_Axes[1]                = (objOrientation * glm::vec3(0.0f, 1.0f, 0.0f)); // Y - Axis
            m_Axes[2]                = (objOrientation * glm::vec3(0.0f, 0.0f, 1.0f)); // Z - Axis
        }

        return m_Axes;
    }

    std::vector<CollisionEdge>& CuboidCollisionShape::GetEdges(const RigidBody3D* currentObject)
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        {
            glm::mat4 transform = currentObject->GetWorldSpaceTransform() * m_LocalTransform;
            for(unsigned int i = 0; i < m_CubeHull->GetNumEdges(); ++i)
            {
                const HullEdge& edge = m_CubeHull->GetEdge(i);
                glm::vec3 A          = transform * glm::vec4(m_CubeHull->GetVertex(edge.vStart).pos, 1.0f);
                glm::vec3 B          = transform * glm::vec4(m_CubeHull->GetVertex(edge.vEnd).pos, 1.0f);

                m_Edges[i] = { A, B };
            }
        }
        return m_Edges;
    }

    void CuboidCollisionShape::GetMinMaxVertexOnAxis(const RigidBody3D* currentObject, const glm::vec3& axis, glm::vec3* out_min, glm::vec3* out_max) const
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        glm::mat4 wsTransform      = currentObject ? currentObject->GetWorldSpaceTransform() * m_LocalTransform : m_LocalTransform;
        const glm::vec3 local_axis = glm::transpose(glm::mat3(wsTransform)) * axis;

        int vMin, vMax;

        m_CubeHull->GetMinMaxVerticesInAxis(local_axis, &vMin, &vMax);

        if(out_min)
            *out_min = wsTransform * glm::vec4(m_CubeHull->GetVertex(vMin).pos, 1.0f);
        if(out_max)
            *out_max = wsTransform * glm::vec4(m_CubeHull->GetVertex(vMax).pos, 1.0f);
    }

    void CuboidCollisionShape::GetIncidentReferencePolygon(const RigidBody3D* currentObject,
                                                           const glm::vec3& axis,
                                                           ReferencePolygon& refPolygon) const
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        glm::mat4 wsTransform = currentObject ? currentObject->GetWorldSpaceTransform() * m_LocalTransform : m_LocalTransform;

        const glm::mat3 invNormalMatrix = glm::transpose(glm::mat3(wsTransform));
        const glm::mat3 normalMatrix    = glm::inverse(invNormalMatrix);

        const glm::vec3 local_axis = invNormalMatrix * axis;

        int minVertex, maxVertex;
        m_CubeHull->GetMinMaxVerticesInAxis(local_axis, &minVertex, &maxVertex);

        const HullVertex& vert = m_CubeHull->GetVertex(maxVertex);

        const HullFace* best_face = nullptr;
        float best_correlation    = -FLT_MAX;
        for(int faceIdx : vert.enclosing_faces)
        {
            const HullFace* face         = &m_CubeHull->GetFace(faceIdx);
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
                const HullVertex& vertex                 = m_CubeHull->GetVertex(vertIdx);
                refPolygon.Faces[refPolygon.FaceCount++] = wsTransform * glm::vec4(vertex.pos, 1.0f);
            }
        }

        if(best_face)
        {
            // Add the reference face itself to the list of adjacent planes
            glm::vec3 wsPointOnPlane = wsTransform * glm::vec4(m_CubeHull->GetVertex(m_CubeHull->GetEdge(best_face->edge_ids[0]).vStart).pos, 1.0f);
            glm::vec3 planeNrml      = -(normalMatrix * best_face->normal);
            planeNrml                = glm::normalize(planeNrml);
            float planeDist          = -glm::dot(planeNrml, wsPointOnPlane);

            refPolygon.AdjacentPlanes[refPolygon.PlaneCount++] = Plane(planeNrml, planeDist);

            for(int edgeIdx : best_face->edge_ids)
            {
                const HullEdge& edge = m_CubeHull->GetEdge(edgeIdx);

                wsPointOnPlane = wsTransform * glm::vec4(m_CubeHull->GetVertex(edge.vStart).pos, 1.0f);

                for(int adjFaceIdx : edge.enclosing_faces)
                {
                    if(adjFaceIdx != best_face->idx)
                    {
                        const HullFace& adjFace = m_CubeHull->GetFace(adjFaceIdx);

                        planeNrml = -(normalMatrix * adjFace.normal);
                        planeNrml = glm::normalize(planeNrml);
                        planeDist = -glm::dot(planeNrml, wsPointOnPlane);

                        refPolygon.AdjacentPlanes[refPolygon.PlaneCount++] = Plane(planeNrml, planeDist);
                    }
                }
            }
        }
    }

    void CuboidCollisionShape::DebugDraw(const RigidBody3D* currentObject) const
    {
        glm::mat4 transform = currentObject->GetWorldSpaceTransform() * m_LocalTransform;

        if(m_CubeHull->GetNumVertices() == 0)
        {
            ConstructCubeHull();
        }

        m_CubeHull->DebugDraw(transform);
    }

    void CuboidCollisionShape::ConstructCubeHull()
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        // Vertices
        // m_CubeHull->AddVertex(glm::vec3(-1.0f, -1.0f, -1.0f)); // 0
        // m_CubeHull->AddVertex(glm::vec3(-1.0f, 1.0f, -1.0f));  // 1
        // m_CubeHull->AddVertex(glm::vec3(1.0f, 1.0f, -1.0f));   // 2
        // m_CubeHull->AddVertex(glm::vec3(1.0f, -1.0f, -1.0f));  // 3

        // m_CubeHull->AddVertex(glm::vec3(-1.0f, -1.0f, 1.0f)); // 4
        // m_CubeHull->AddVertex(glm::vec3(-1.0f, 1.0f, 1.0f));  // 5
        // m_CubeHull->AddVertex(glm::vec3(1.0f, 1.0f, 1.0f));   // 6
        // m_CubeHull->AddVertex(glm::vec3(1.0f, -1.0f, 1.0f));  // 7

        // int face1[] = { 0, 1, 2, 3 };
        // int face2[] = { 7, 6, 5, 4 };
        // int face3[] = { 5, 6, 2, 1 };
        // int face4[] = { 0, 3, 7, 4 };
        // int face5[] = { 6, 7, 3, 2 };
        // int face6[] = { 4, 5, 1, 0 };

        //// Faces
        // m_CubeHull->AddFace(glm::vec3(0.0f, 0.0f, -1.0f), 4, face1);
        // m_CubeHull->AddFace(glm::vec3(0.0f, 0.0f, 1.0f), 4, face2);
        // m_CubeHull->AddFace(glm::vec3(0.0f, 1.0f, 0.0f), 4, face3);
        // m_CubeHull->AddFace(glm::vec3(0.0f, -1.0f, 0.0f), 4, face4);
        // m_CubeHull->AddFace(glm::vec3(1.0f, 0.0f, 0.0f), 4, face5);
        // m_CubeHull->AddFace(glm::vec3(-1.0f, 0.0f, 0.0f), 4, face6);
    }
}
