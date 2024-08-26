#include "Precompiled.h"
#include "PyramidCollisionShape.h"
#include "Physics/LumosPhysicsEngine/RigidBody3D.h"

namespace Lumos
{

    UniquePtr<Hull> PyramidCollisionShape::m_PyramidHull = CreateUniquePtr<Hull>();

    PyramidCollisionShape::PyramidCollisionShape()
    {
        m_PyramidHalfDimensions = Vec3(0.5f, 0.5f, 0.5f);
        m_Type                  = CollisionShapeType::CollisionPyramid;
        m_LocalTransform        = Mat4::Scale(m_PyramidHalfDimensions);

        if(m_PyramidHull->GetNumVertices() == 0)
        {
            ConstructPyramidHull();
        }

        m_Axes.Resize(5);
        m_Edges.Resize(m_PyramidHull->GetNumEdges());
    }

    PyramidCollisionShape::PyramidCollisionShape(const Vec3& halfdims)
    {
        LUMOS_PROFILE_FUNCTION();
        m_PyramidHalfDimensions = halfdims;

        m_LocalTransform = Mat4::Scale(m_PyramidHalfDimensions);
        m_Type           = CollisionShapeType::CollisionPyramid;

        Vec3 m_Points[5] = {
            m_LocalTransform * Vec4(-1.0f, -1.0f, -1.0f, 1.0f),
            m_LocalTransform * Vec4(-1.0f, -1.0f, 1.0f, 1.0f),
            m_LocalTransform * Vec4(1.0f, -1.0f, 1.0f, 1.0f),
            m_LocalTransform * Vec4(1.0f, -1.0f, -1.0f, 1.0f),
            m_LocalTransform * Vec4(0.0f, 1.0f, 0.0f, 1.0f)
        };

        m_Normals[0] = (Maths::Cross(m_Points[0] - m_Points[3], m_Points[4] - m_Points[3])).Normalised();
        m_Normals[1] = (Maths::Cross(m_Points[1] - m_Points[0], m_Points[4] - m_Points[0])).Normalised();
        m_Normals[2] = (Maths::Cross(m_Points[2] - m_Points[1], m_Points[4] - m_Points[1])).Normalised();
        m_Normals[3] = (Maths::Cross(m_Points[3] - m_Points[2], m_Points[4] - m_Points[2])).Normalised();
        m_Normals[4] = Vec3(0.0f, -1.0f, 0.0f);

        if(m_PyramidHull->GetNumVertices() == 0)
        {
            ConstructPyramidHull();
        }

        m_Axes.Resize(5);
        m_Edges.Resize(m_PyramidHull->GetNumEdges());
    }

    PyramidCollisionShape::~PyramidCollisionShape()
    {
    }

    Mat3 PyramidCollisionShape::BuildInverseInertia(float invMass) const
    {
        LUMOS_PROFILE_FUNCTION();
        Vec3 scaleSq = m_PyramidHalfDimensions + m_PyramidHalfDimensions;
        scaleSq      = scaleSq * scaleSq;

        Mat3 inertia(1.0f);
        inertia.SetDiagonal(Vec3(3.0f * invMass / ((0.2f * scaleSq.x) + (0.15f * scaleSq.y)), 3.0f * invMass / ((0.2f * scaleSq.z) + (0.15f * scaleSq.y)), 3.0f * invMass / ((0.2f * scaleSq.z) + (0.2f * scaleSq.x))));
        return inertia;
    }

    TDArray<CollisionEdge>& PyramidCollisionShape::GetEdges(const RigidBody3D* currentObject)
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        {
            Mat4 transform = currentObject->GetWorldSpaceTransform() * m_LocalTransform;
            for(unsigned int i = 0; i < m_PyramidHull->GetNumEdges(); ++i)
            {
                const HullEdge& edge = m_PyramidHull->GetEdge(i);
                Vec3 A               = transform * Vec4(m_PyramidHull->GetVertex(edge.vStart).pos, 1.0f);
                Vec3 B               = transform * Vec4(m_PyramidHull->GetVertex(edge.vEnd).pos, 1.0f);

                m_Edges[i] = { A, B };
            }
        }
        return m_Edges;
    }

    TDArray<Vec3>& PyramidCollisionShape::GetCollisionAxes(const RigidBody3D* currentObject)
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        {
            m_Axes.Clear();
            m_Axes.Resize(5);
            const Mat3 objOrientation = Mat3(currentObject->GetOrientation());
            m_Axes[0]                 = (objOrientation * m_Normals[0]);
            m_Axes[1]                 = (objOrientation * m_Normals[1]);
            m_Axes[2]                 = (objOrientation * m_Normals[2]);
            m_Axes[3]                 = (objOrientation * m_Normals[3]);
            m_Axes[4]                 = (objOrientation * m_Normals[4]);
        }

        return m_Axes;
    }

    void PyramidCollisionShape::GetMinMaxVertexOnAxis(const RigidBody3D* currentObject, const Vec3& axis, Vec3* out_min, Vec3* out_max) const
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        Mat4 wsTransform      = currentObject ? currentObject->GetWorldSpaceTransform() * m_LocalTransform : m_LocalTransform;
        const Vec3 local_axis = Maths::Transpose(wsTransform) * Vec4(axis, 1.0f);

        int vMin, vMax;
        m_PyramidHull->GetMinMaxVerticesInAxis(local_axis, &vMin, &vMax);

        if(out_min)
            *out_min = wsTransform * Vec4(m_PyramidHull->GetVertex(vMin).pos, 1.0f);
        if(out_max)
            *out_max = wsTransform * Vec4(m_PyramidHull->GetVertex(vMax).pos, 1.0f);
    }

    void PyramidCollisionShape::GetIncidentReferencePolygon(const RigidBody3D* currentObject,
                                                            const Vec3& axis,
                                                            ReferencePolygon& refPolygon) const
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        Mat4 wsTransform = currentObject ? currentObject->GetWorldSpaceTransform() * m_LocalTransform : m_LocalTransform;

        const Mat3 invNormalMatrix = Mat4::Inverse(wsTransform);
        const Mat3 normalMatrix    = Maths::Transpose(invNormalMatrix);

        const Vec3 local_axis = invNormalMatrix * axis;

        int minVertex, maxVertex;
        m_PyramidHull->GetMinMaxVerticesInAxis(local_axis, &minVertex, &maxVertex);

        const HullVertex& vert = m_PyramidHull->GetVertex(maxVertex);

        const HullFace* best_face = nullptr;
        float best_correlation    = -FLT_MAX;
        for(int faceIdx : vert.enclosing_faces)
        {
            const HullFace* face         = &m_PyramidHull->GetFace(faceIdx);
            const float temp_correlation = Maths::Dot(local_axis, face->normal);
            if(temp_correlation > best_correlation)
            {
                best_correlation = temp_correlation;
                best_face        = face;
            }
        }

        {
            if(best_face)
                refPolygon.Normal = normalMatrix * best_face->normal;
            refPolygon.Normal.Normalise();
        }

        if(best_face)
        {
            for(int vertIdx : best_face->vert_ids)
            {
                const HullVertex& vertex                 = m_PyramidHull->GetVertex(vertIdx);
                refPolygon.Faces[refPolygon.FaceCount++] = wsTransform * Vec4(vertex.pos, 1.0f);
            }
        }

        if(best_face)
        {
            // Add the reference face itself to the list of adjacent planes
            Vec3 wsPointOnPlane = wsTransform * Vec4(m_PyramidHull->GetVertex(m_PyramidHull->GetEdge(best_face->edge_ids[0]).vStart).pos, 1.0f);
            Vec3 planeNrml      = -(normalMatrix * best_face->normal);
            planeNrml.Normalise();
            float planeDist = -Maths::Dot(planeNrml, wsPointOnPlane);

            refPolygon.AdjacentPlanes[refPolygon.PlaneCount++] = Plane(planeNrml, planeDist);

            for(int edgeIdx : best_face->edge_ids)
            {
                const HullEdge& edge = m_PyramidHull->GetEdge(edgeIdx);

                wsPointOnPlane = wsTransform * Vec4(m_PyramidHull->GetVertex(edge.vStart).pos, 1.0f);

                for(int adjFaceIdx : edge.enclosing_faces)
                {
                    if(adjFaceIdx != best_face->idx)
                    {
                        const HullFace& adjFace = m_PyramidHull->GetFace(adjFaceIdx);

                        planeNrml = -(normalMatrix * adjFace.normal);
                        planeNrml.Normalise();
                        planeDist = -Maths::Dot(planeNrml, wsPointOnPlane);

                        refPolygon.AdjacentPlanes[refPolygon.PlaneCount++] = Plane(planeNrml, planeDist);
                    }
                }
            }
        }
    }

    void PyramidCollisionShape::DebugDraw(const RigidBody3D* currentObject) const
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        const Mat4 transform = currentObject->GetWorldSpaceTransform() * m_LocalTransform;

        m_PyramidHull->DebugDraw(transform);
    }

    void PyramidCollisionShape::ConstructPyramidHull()
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        Vec3 v0 = Vec3(-1.0f, -1.0f, -1.0f); // 0
        Vec3 v1 = Vec3(-1.0f, -1.0f, 1.0f);  // 1
        Vec3 v2 = Vec3(1.0f, -1.0f, 1.0f);   // 2
        Vec3 v3 = Vec3(1.0f, -1.0f, -1.0f);  // 3
        Vec3 v4 = Vec3(0.0f, 1.0f, 0.0f);    // 4
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

        m_PyramidHull->AddFace(Maths::Cross((v0 - v3), v4 - v3).Normalised(), 3, face1);
        m_PyramidHull->AddFace(Maths::Cross(v1 - v0, v4 - v0).Normalised(), 3, face2);
        m_PyramidHull->AddFace(Maths::Cross(v2 - v1, v4 - v1).Normalised(), 3, face3);
        m_PyramidHull->AddFace(Maths::Cross(v3 - v2, v4 - v2).Normalised(), 3, face4);
        m_PyramidHull->AddFace(Vec3(0.0f, -1.0f, 0.0f), 4, face5);
    }
}
