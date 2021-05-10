#include "Precompiled.h"
#include "PyramidCollisionShape.h"
#include "RigidBody3D.h"

namespace Lumos
{

    UniqueRef<Hull> PyramidCollisionShape::m_PyramidHull = CreateUniqueRef<Hull>();

    PyramidCollisionShape::PyramidCollisionShape()
    {
        m_PyramidHalfDimensions = Maths::Vector3(0.5f, 0.5f, 0.5f);
        m_Type = CollisionShapeType::CollisionPyramid;
        m_LocalTransform = Maths::Matrix4::Scale(m_PyramidHalfDimensions);

        if(m_PyramidHull->GetNumVertices() == 0)
        {
            ConstructPyramidHull();
        }

        m_Axes.resize(5);
        m_Edges.resize(m_PyramidHull->GetNumEdges());
    }

    PyramidCollisionShape::PyramidCollisionShape(const Maths::Vector3& halfdims)
    {
        LUMOS_PROFILE_FUNCTION();
        m_PyramidHalfDimensions = halfdims;

        m_LocalTransform = Maths::Matrix4::Scale(m_PyramidHalfDimensions);
        m_Type = CollisionShapeType::CollisionPyramid;

        Maths::Vector3 m_Points[5] = {
            m_LocalTransform * Maths::Vector3(-1.0f, -1.0f, -1.0f),
            m_LocalTransform * Maths::Vector3(-1.0f, -1.0f, 1.0f),
            m_LocalTransform * Maths::Vector3(1.0f, -1.0f, 1.0f),
            m_LocalTransform * Maths::Vector3(1.0f, -1.0f, -1.0f),
            m_LocalTransform * Maths::Vector3(0.0f, 1.0f, 0.0f)
        };

        m_Normals[0] = Maths::Vector3::Cross(m_Points[0] - m_Points[3], m_Points[4] - m_Points[3]).Normalised();
        m_Normals[1] = Maths::Vector3::Cross(m_Points[1] - m_Points[0], m_Points[4] - m_Points[0]).Normalised();
        m_Normals[2] = Maths::Vector3::Cross(m_Points[2] - m_Points[1], m_Points[4] - m_Points[1]).Normalised();
        m_Normals[3] = Maths::Vector3::Cross(m_Points[3] - m_Points[2], m_Points[4] - m_Points[2]).Normalised();
        m_Normals[4] = Maths::Vector3(0.0f, -1.0f, 0.0f);

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

    Maths::Matrix3 PyramidCollisionShape::BuildInverseInertia(float invMass) const
    {
        LUMOS_PROFILE_FUNCTION();
        Maths::Vector3 scaleSq = m_PyramidHalfDimensions * m_PyramidHalfDimensions;

        Maths::Matrix3 inertia;

        inertia.m00_ = invMass / ((0.2f * scaleSq.x) + (0.15f * scaleSq.y));
        inertia.m11_ = invMass / ((0.2f * scaleSq.z) + (0.15f * scaleSq.y));
        inertia.m22_ = invMass / ((0.2f * scaleSq.z) + (0.2f * scaleSq.x));

        return inertia;
    }

    std::vector<CollisionEdge>& PyramidCollisionShape::GetEdges(const RigidBody3D* currentObject)
    {
        LUMOS_PROFILE_FUNCTION();
        {
            Maths::Matrix4 transform = currentObject->GetWorldSpaceTransform() * m_LocalTransform;
            for(unsigned int i = 0; i < m_PyramidHull->GetNumEdges(); ++i)
            {
                const HullEdge& edge = m_PyramidHull->GetEdge(i);
                Maths::Vector3 A = transform * m_PyramidHull->GetVertex(edge.vStart).pos;
                Maths::Vector3 B = transform * m_PyramidHull->GetVertex(edge.vEnd).pos;

                m_Edges[i] = { A, B };
            }
        }
        return m_Edges;
    }

    std::vector<Maths::Vector3>& PyramidCollisionShape::GetCollisionAxes(const RigidBody3D* currentObject)
    {
        LUMOS_PROFILE_FUNCTION();
        {
            m_Axes.clear();
            m_Axes.resize(5);
            const Maths::Matrix3 objOrientation = currentObject->GetOrientation().RotationMatrix();
            m_Axes[0] = (objOrientation * m_Normals[0]);
            m_Axes[1] = (objOrientation * m_Normals[1]);
            m_Axes[2] = (objOrientation * m_Normals[2]);
            m_Axes[3] = (objOrientation * m_Normals[3]);
            m_Axes[4] = (objOrientation * m_Normals[4]);
        }

        return m_Axes;
    }

    void PyramidCollisionShape::GetMinMaxVertexOnAxis(const RigidBody3D* currentObject, const Maths::Vector3& axis, Maths::Vector3* out_min, Maths::Vector3* out_max) const
    {
        LUMOS_PROFILE_FUNCTION();
        Maths::Matrix4 wsTransform = currentObject ? currentObject->GetWorldSpaceTransform() * m_LocalTransform : m_LocalTransform;
        const Maths::Vector3 local_axis = wsTransform.ToMatrix3().Transpose() * axis;

        int vMin, vMax;
        m_PyramidHull->GetMinMaxVerticesInAxis(local_axis, &vMin, &vMax);

        if(out_min)
            *out_min = wsTransform * m_PyramidHull->GetVertex(vMin).pos;
        if(out_max)
            *out_max = wsTransform * m_PyramidHull->GetVertex(vMax).pos;
    }

    void PyramidCollisionShape::GetIncidentReferencePolygon(const RigidBody3D* currentObject,
        const Maths::Vector3& axis,
        ReferencePolygon& refPolygon) const
    {
        LUMOS_PROFILE_FUNCTION();
        Maths::Matrix4 wsTransform = currentObject ? currentObject->GetWorldSpaceTransform() * m_LocalTransform : m_LocalTransform;

        const Maths::Matrix3 invNormalMatrix = wsTransform.ToMatrix3().Inverse();
        const Maths::Matrix3 normalMatrix = invNormalMatrix.Transpose();

        const Maths::Vector3 local_axis = invNormalMatrix * axis;

        int minVertex, maxVertex;
        m_PyramidHull->GetMinMaxVerticesInAxis(local_axis, &minVertex, &maxVertex);

        const HullVertex& vert = m_PyramidHull->GetVertex(maxVertex);

        const HullFace* best_face = nullptr;
        float best_correlation = -FLT_MAX;
        for(int faceIdx : vert.enclosing_faces)
        {
            const HullFace* face = &m_PyramidHull->GetFace(faceIdx);
            const float temp_correlation = Maths::Vector3::Dot(local_axis, face->normal);
            if(temp_correlation > best_correlation)
            {
                best_correlation = temp_correlation;
                best_face = face;
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
                const HullVertex& vertex = m_PyramidHull->GetVertex(vertIdx);
                refPolygon.Faces[refPolygon.FaceCount++] = wsTransform * vertex.pos;
            }
        }

        if(best_face)
        {
            //Add the reference face itself to the list of adjacent planes
            Maths::Vector3 wsPointOnPlane = wsTransform * m_PyramidHull->GetVertex(m_PyramidHull->GetEdge(best_face->edge_ids[0]).vStart).pos;
            Maths::Vector3 planeNrml = -(normalMatrix * best_face->normal);
            planeNrml.Normalise();
            float planeDist = -Maths::Vector3::Dot(planeNrml, wsPointOnPlane);

            refPolygon.AdjacentPlanes[refPolygon.PlaneCount++] = { planeNrml, planeDist };

            for(int edgeIdx : best_face->edge_ids)
            {
                const HullEdge& edge = m_PyramidHull->GetEdge(edgeIdx);

                wsPointOnPlane = wsTransform * m_PyramidHull->GetVertex(edge.vStart).pos;

                for(int adjFaceIdx : edge.enclosing_faces)
                {
                    if(adjFaceIdx != best_face->idx)
                    {
                        const HullFace& adjFace = m_PyramidHull->GetFace(adjFaceIdx);

                        planeNrml = -(normalMatrix * adjFace.normal);
                        planeNrml.Normalise();
                        planeDist = -Maths::Vector3::Dot(planeNrml, wsPointOnPlane);

                        refPolygon.AdjacentPlanes[refPolygon.PlaneCount++] = { planeNrml, planeDist };
                    }
                }
            }
        }
    }

    void PyramidCollisionShape::DebugDraw(const RigidBody3D* currentObject) const
    {
        LUMOS_PROFILE_FUNCTION();
        const Maths::Matrix4 transform = currentObject->GetWorldSpaceTransform() * m_LocalTransform;

        m_PyramidHull->DebugDraw(transform);
    }

    void PyramidCollisionShape::ConstructPyramidHull()
    {
        LUMOS_PROFILE_FUNCTION();
        Maths::Vector3 v0 = Maths::Vector3(-1.0f, -1.0f, -1.0f); // 0
        Maths::Vector3 v1 = Maths::Vector3(-1.0f, -1.0f, 1.0f); // 1
        Maths::Vector3 v2 = Maths::Vector3(1.0f, -1.0f, 1.0f); // 2
        Maths::Vector3 v3 = Maths::Vector3(1.0f, -1.0f, -1.0f); // 3
        Maths::Vector3 v4 = Maths::Vector3(0.0f, 1.0f, 0.0f); // 4
        //Vertices
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

        m_PyramidHull->AddFace(Maths::Vector3::Cross((v0 - v3), v4 - v3).Normalised(), 3, face1);
        m_PyramidHull->AddFace(Maths::Vector3::Cross(v1 - v0, v4 - v0).Normalised(), 3, face2);
        m_PyramidHull->AddFace(Maths::Vector3::Cross(v2 - v1, v4 - v1).Normalised(), 3, face3);
        m_PyramidHull->AddFace(Maths::Vector3::Cross(v3 - v2, v4 - v2).Normalised(), 3, face4);
        m_PyramidHull->AddFace(Maths::Vector3(0.0f, -1.0f, 0.0f), 4, face5);
    }
}
