#ifndef LUMOS_PLATFORM_MACOS
#include "Precompiled.h"
#endif
#include "PyramidCollisionShape.h"
#include "Physics/LumosPhysicsEngine/RigidBody3D.h"

namespace Lumos
{

    UniquePtr<Hull> PyramidCollisionShape::m_PyramidHull = CreateUniquePtr<Hull>();

    PyramidCollisionShape::PyramidCollisionShape()
    {
        m_PyramidHalfDimensions = glm::vec3(0.5f, 0.5f, 0.5f);
        m_Type                  = CollisionShapeType::CollisionPyramid;
        m_LocalTransform        = glm::scale(glm::mat4(1.0), m_PyramidHalfDimensions);

        if(m_PyramidHull->GetNumVertices() == 0)
        {
            ConstructPyramidHull();
        }

        m_Axes.resize(5);
        m_Edges.resize(m_PyramidHull->GetNumEdges());
    }

    PyramidCollisionShape::PyramidCollisionShape(const glm::vec3& halfdims)
    {
        LUMOS_PROFILE_FUNCTION();
        m_PyramidHalfDimensions = halfdims;

        m_LocalTransform = glm::scale(glm::mat4(1.0), m_PyramidHalfDimensions);
        m_Type           = CollisionShapeType::CollisionPyramid;

        glm::vec3 m_Points[5] = {
            m_LocalTransform * glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f),
            m_LocalTransform * glm::vec4(-1.0f, -1.0f, 1.0f, 1.0f),
            m_LocalTransform * glm::vec4(1.0f, -1.0f, 1.0f, 1.0f),
            m_LocalTransform * glm::vec4(1.0f, -1.0f, -1.0f, 1.0f),
            m_LocalTransform * glm::vec4(0.0f, 1.0f, 0.0f, 1.0f)
        };

        m_Normals[0] = glm::normalize(glm::cross(m_Points[0] - m_Points[3], m_Points[4] - m_Points[3]));
        m_Normals[1] = glm::normalize(glm::cross(m_Points[1] - m_Points[0], m_Points[4] - m_Points[0]));
        m_Normals[2] = glm::normalize(glm::cross(m_Points[2] - m_Points[1], m_Points[4] - m_Points[1]));
        m_Normals[3] = glm::normalize(glm::cross(m_Points[3] - m_Points[2], m_Points[4] - m_Points[2]));
        m_Normals[4] = glm::vec3(0.0f, -1.0f, 0.0f);

        if(m_PyramidHull->GetNumVertices() == 0)
        {
            ConstructPyramidHull();
        }

        m_Axes.resize(5);
        m_Edges.resize(m_PyramidHull->GetNumEdges());
    }

    PyramidCollisionShape::~PyramidCollisionShape()
    {
    }

    glm::mat3 PyramidCollisionShape::BuildInverseInertia(float invMass) const
    {
        LUMOS_PROFILE_FUNCTION();
        glm::vec3 scaleSq = m_PyramidHalfDimensions + m_PyramidHalfDimensions;
        scaleSq           = scaleSq * scaleSq;

        glm::mat3 inertia(1.0f);

        inertia[0][0] = 3.0f * invMass / ((0.2f * scaleSq.x) + (0.15f * scaleSq.y));
        inertia[1][1] = 3.0f * invMass / ((0.2f * scaleSq.z) + (0.15f * scaleSq.y));
        inertia[2][2] = 3.0f * invMass / ((0.2f * scaleSq.z) + (0.2f * scaleSq.x));

        return inertia;
    }

    std::vector<CollisionEdge>& PyramidCollisionShape::GetEdges(const RigidBody3D* currentObject)
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        {
            glm::mat4 transform = currentObject->GetWorldSpaceTransform() * m_LocalTransform;
            for(unsigned int i = 0; i < m_PyramidHull->GetNumEdges(); ++i)
            {
                const HullEdge& edge = m_PyramidHull->GetEdge(i);
                glm::vec3 A          = transform * glm::vec4(m_PyramidHull->GetVertex(edge.vStart).pos, 1.0f);
                glm::vec3 B          = transform * glm::vec4(m_PyramidHull->GetVertex(edge.vEnd).pos, 1.0f);

                m_Edges[i] = { A, B };
            }
        }
        return m_Edges;
    }

    std::vector<glm::vec3>& PyramidCollisionShape::GetCollisionAxes(const RigidBody3D* currentObject)
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        {
            m_Axes.clear();
            m_Axes.resize(5);
            const glm::mat3 objOrientation = glm::toMat3(currentObject->GetOrientation());
            m_Axes[0]                      = (objOrientation * m_Normals[0]);
            m_Axes[1]                      = (objOrientation * m_Normals[1]);
            m_Axes[2]                      = (objOrientation * m_Normals[2]);
            m_Axes[3]                      = (objOrientation * m_Normals[3]);
            m_Axes[4]                      = (objOrientation * m_Normals[4]);
        }

        return m_Axes;
    }

    void PyramidCollisionShape::GetMinMaxVertexOnAxis(const RigidBody3D* currentObject, const glm::vec3& axis, glm::vec3* out_min, glm::vec3* out_max) const
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        glm::mat4 wsTransform      = currentObject ? currentObject->GetWorldSpaceTransform() * m_LocalTransform : m_LocalTransform;
        const glm::vec3 local_axis = glm::transpose(wsTransform) * glm::vec4(axis, 1.0f);

        int vMin, vMax;
        m_PyramidHull->GetMinMaxVerticesInAxis(local_axis, &vMin, &vMax);

        if(out_min)
            *out_min = wsTransform * glm::vec4(m_PyramidHull->GetVertex(vMin).pos, 1.0f);
        if(out_max)
            *out_max = wsTransform * glm::vec4(m_PyramidHull->GetVertex(vMax).pos, 1.0f);
    }

    void PyramidCollisionShape::GetIncidentReferencePolygon(const RigidBody3D* currentObject,
                                                            const glm::vec3& axis,
                                                            ReferencePolygon& refPolygon) const
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        glm::mat4 wsTransform = currentObject ? currentObject->GetWorldSpaceTransform() * m_LocalTransform : m_LocalTransform;

        const glm::mat3 invNormalMatrix = glm::inverse(wsTransform);
        const glm::mat3 normalMatrix    = glm::transpose(invNormalMatrix);

        const glm::vec3 local_axis = invNormalMatrix * axis;

        int minVertex, maxVertex;
        m_PyramidHull->GetMinMaxVerticesInAxis(local_axis, &minVertex, &maxVertex);

        const HullVertex& vert = m_PyramidHull->GetVertex(maxVertex);

        const HullFace* best_face = nullptr;
        float best_correlation    = -FLT_MAX;
        for(int faceIdx : vert.enclosing_faces)
        {
            const HullFace* face         = &m_PyramidHull->GetFace(faceIdx);
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
                const HullVertex& vertex                 = m_PyramidHull->GetVertex(vertIdx);
                refPolygon.Faces[refPolygon.FaceCount++] = wsTransform * glm::vec4(vertex.pos, 1.0f);
            }
        }

        if(best_face)
        {
            // Add the reference face itself to the list of adjacent planes
            glm::vec3 wsPointOnPlane = wsTransform * glm::vec4(m_PyramidHull->GetVertex(m_PyramidHull->GetEdge(best_face->edge_ids[0]).vStart).pos, 1.0f);
            glm::vec3 planeNrml      = -(normalMatrix * best_face->normal);
            planeNrml                = glm::normalize(planeNrml);
            float planeDist          = -glm::dot(planeNrml, wsPointOnPlane);

            refPolygon.AdjacentPlanes[refPolygon.PlaneCount++] = Plane(planeNrml, planeDist);

            for(int edgeIdx : best_face->edge_ids)
            {
                const HullEdge& edge = m_PyramidHull->GetEdge(edgeIdx);

                wsPointOnPlane = wsTransform * glm::vec4(m_PyramidHull->GetVertex(edge.vStart).pos, 1.0f);

                for(int adjFaceIdx : edge.enclosing_faces)
                {
                    if(adjFaceIdx != best_face->idx)
                    {
                        const HullFace& adjFace = m_PyramidHull->GetFace(adjFaceIdx);

                        planeNrml = -(normalMatrix * adjFace.normal);
                        planeNrml = glm::normalize(planeNrml);
                        planeDist = -glm::dot(planeNrml, wsPointOnPlane);

                        refPolygon.AdjacentPlanes[refPolygon.PlaneCount++] = Plane(planeNrml, planeDist);
                    }
                }
            }
        }
    }

    void PyramidCollisionShape::DebugDraw(const RigidBody3D* currentObject) const
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        const glm::mat4 transform = currentObject->GetWorldSpaceTransform() * m_LocalTransform;

        m_PyramidHull->DebugDraw(transform);
    }

    void PyramidCollisionShape::ConstructPyramidHull()
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        glm::vec3 v0 = glm::vec3(-1.0f, -1.0f, -1.0f); // 0
        glm::vec3 v1 = glm::vec3(-1.0f, -1.0f, 1.0f);  // 1
        glm::vec3 v2 = glm::vec3(1.0f, -1.0f, 1.0f);   // 2
        glm::vec3 v3 = glm::vec3(1.0f, -1.0f, -1.0f);  // 3
        glm::vec3 v4 = glm::vec3(0.0f, 1.0f, 0.0f);    // 4
        // Vertices
        m_PyramidHull->AddVertex(v0); // 0
        m_PyramidHull->AddVertex(v1); // 1
        m_PyramidHull->AddVertex(v2); // 2
        m_PyramidHull->AddVertex(v3); // 3
        m_PyramidHull->AddVertex(v4); // 4

        int face1[] = { 0, 4, 3 };
        int face2[] = { 1, 4, 0 };
        int face3[] = { 2, 4, 1 };
        int face4[] = { 3, 4, 2 };
        int face5[] = { 0, 3, 2, 1 };

        m_PyramidHull->AddFace(glm::normalize(glm::cross((v0 - v3), v4 - v3)), 3, face1);
        m_PyramidHull->AddFace(glm::normalize(glm::cross(v1 - v0, v4 - v0)), 3, face2);
        m_PyramidHull->AddFace(glm::normalize(glm::cross(v2 - v1, v4 - v1)), 3, face3);
        m_PyramidHull->AddFace(glm::normalize(glm::cross(v3 - v2, v4 - v2)), 3, face4);
        m_PyramidHull->AddFace(glm::normalize(glm::vec3(0.0f, -1.0f, 0.0f)), 4, face5);
    }
}
