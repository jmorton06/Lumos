#include "Precompiled.h"
#include "CuboidCollisionShape.h"
#include "RigidBody3D.h"
#include "Maths/Matrix3.h"

namespace Lumos
{

    SharedRef<Hull> CuboidCollisionShape::m_CubeHull = CreateSharedRef<Hull>();

    CuboidCollisionShape::CuboidCollisionShape()
    {
        m_CuboidHalfDimensions = Maths::Vector3(0.5f, 0.5f, 0.5f);
        m_Type = CollisionShapeType::CollisionCuboid;

        if(m_CubeHull->GetNumVertices() == 0)
        {
            ConstructCubeHull();
        }

        m_Axes.resize(3);
        m_Edges.resize(m_CubeHull->GetNumEdges());
    }

    CuboidCollisionShape::CuboidCollisionShape(const Maths::Vector3& halfdims)
    {
        m_CuboidHalfDimensions = halfdims;
        m_LocalTransform = Maths::Matrix4::Scale(halfdims);
        m_Type = CollisionShapeType::CollisionCuboid;

        if(m_CubeHull->GetNumVertices() == 0)
        {
            ConstructCubeHull();
        }

        m_Axes.resize(3);
        m_Edges.resize(m_CubeHull->GetNumEdges());
    }

    CuboidCollisionShape::~CuboidCollisionShape()
    {
    }

    Maths::Matrix3 CuboidCollisionShape::BuildInverseInertia(float invMass) const
    {
        LUMOS_PROFILE_FUNCTION();
        Maths::Matrix3 inertia;

        Maths::Vector3 dimsSq = (m_CuboidHalfDimensions + m_CuboidHalfDimensions);
        dimsSq = dimsSq * dimsSq;

        inertia.m00_ = 12.f * invMass * 1.f / (dimsSq.y + dimsSq.z);
        inertia.m11_ = 12.f * invMass * 1.f / (dimsSq.x + dimsSq.z);
        inertia.m22_ = 12.f * invMass * 1.f / (dimsSq.x + dimsSq.y);

        return inertia;
    }

    std::vector<Maths::Vector3>& CuboidCollisionShape::GetCollisionAxes(const RigidBody3D* currentObject)
    {
        LUMOS_PROFILE_FUNCTION();
        {
            m_Axes.resize(3);

            Maths::Matrix3 objOrientation = currentObject->GetOrientation().RotationMatrix();
            m_Axes[0] = (objOrientation * Maths::Vector3(1.0f, 0.0f, 0.0f)); //X - Axis
            m_Axes[1] = (objOrientation * Maths::Vector3(0.0f, 1.0f, 0.0f)); //Y - Axis
            m_Axes[2] = (objOrientation * Maths::Vector3(0.0f, 0.0f, 1.0f)); //Z - Axis
        }

        return m_Axes;
    }

    std::vector<CollisionEdge>& CuboidCollisionShape::GetEdges(const RigidBody3D* currentObject)
    {
        LUMOS_PROFILE_FUNCTION();
        {
            Maths::Matrix4 transform = currentObject->GetWorldSpaceTransform() * m_LocalTransform;
            for(unsigned int i = 0; i < m_CubeHull->GetNumEdges(); ++i)
            {
                const HullEdge& edge = m_CubeHull->GetEdge(i);
                Maths::Vector3 A = transform * m_CubeHull->GetVertex(edge.vStart).pos;
                Maths::Vector3 B = transform * m_CubeHull->GetVertex(edge.vEnd).pos;

                m_Edges[i] = { A, B };
            }
        }
        return m_Edges;
    }

    void CuboidCollisionShape::GetMinMaxVertexOnAxis(const RigidBody3D* currentObject, const Maths::Vector3& axis, Maths::Vector3* out_min, Maths::Vector3* out_max) const
    {
        LUMOS_PROFILE_FUNCTION();
        Maths::Matrix4 wsTransform = currentObject ? currentObject->GetWorldSpaceTransform() * m_LocalTransform : m_LocalTransform;
        const Maths::Vector3 local_axis = wsTransform.ToMatrix3().Transpose() * axis;

        int vMin, vMax;

        m_CubeHull->GetMinMaxVerticesInAxis(local_axis, &vMin, &vMax);

        if(out_min)
            *out_min = wsTransform * m_CubeHull->GetVertex(vMin).pos;
        if(out_max)
            *out_max = wsTransform * m_CubeHull->GetVertex(vMax).pos;
    }

    void CuboidCollisionShape::GetIncidentReferencePolygon(const RigidBody3D* currentObject,
        const Maths::Vector3& axis,
        ReferencePolygon& refPolygon) const
    {
        LUMOS_PROFILE_FUNCTION();
        Maths::Matrix4 wsTransform = currentObject ? currentObject->GetWorldSpaceTransform() * m_LocalTransform : m_LocalTransform;

        const Maths::Matrix3 invNormalMatrix = wsTransform.ToMatrix3().Inverse();
        const Maths::Matrix3 normalMatrix = invNormalMatrix.Transpose();

        const Maths::Vector3 local_axis = invNormalMatrix * axis;

        int minVertex, maxVertex;
        m_CubeHull->GetMinMaxVerticesInAxis(local_axis, &minVertex, &maxVertex);

        const HullVertex& vert = m_CubeHull->GetVertex(maxVertex);

        const HullFace* best_face = nullptr;
        float best_correlation = -FLT_MAX;
        for(int faceIdx : vert.enclosing_faces)
        {
            const HullFace* face = &m_CubeHull->GetFace(faceIdx);
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
                const HullVertex& vertex = m_CubeHull->GetVertex(vertIdx);
                refPolygon.Faces[refPolygon.FaceCount++] = wsTransform * vertex.pos;
            }
        }

        if(best_face)
        {
            //Add the reference face itself to the list of adjacent planes
            Maths::Vector3 wsPointOnPlane = wsTransform * m_CubeHull->GetVertex(m_CubeHull->GetEdge(best_face->edge_ids[0]).vStart).pos;
            Maths::Vector3 planeNrml = -(normalMatrix * best_face->normal);
            planeNrml.Normalise();
            float planeDist = -Maths::Vector3::Dot(planeNrml, wsPointOnPlane);

            refPolygon.AdjacentPlanes[refPolygon.PlaneCount++] = { planeNrml, planeDist };

            for(int edgeIdx : best_face->edge_ids)
            {
                const HullEdge& edge = m_CubeHull->GetEdge(edgeIdx);

                wsPointOnPlane = wsTransform * m_CubeHull->GetVertex(edge.vStart).pos;

                for(int adjFaceIdx : edge.enclosing_faces)
                {
                    if(adjFaceIdx != best_face->idx)
                    {
                        const HullFace& adjFace = m_CubeHull->GetFace(adjFaceIdx);

                        planeNrml = -(normalMatrix * adjFace.normal);
                        planeNrml.Normalise();
                        planeDist = -Maths::Vector3::Dot(planeNrml, wsPointOnPlane);

                        refPolygon.AdjacentPlanes[refPolygon.PlaneCount++] = { planeNrml, planeDist };
                    }
                }
            }
        }
    }

    void CuboidCollisionShape::DebugDraw(const RigidBody3D* currentObject) const
    {
        Maths::Matrix4 transform = currentObject->GetWorldSpaceTransform() * m_LocalTransform;

        if(m_CubeHull->GetNumVertices() == 0)
        {
            ConstructCubeHull();
        }

        m_CubeHull->DebugDraw(transform);
    }

    void CuboidCollisionShape::ConstructCubeHull()
    {
        LUMOS_PROFILE_FUNCTION();
        //Vertices
        m_CubeHull->AddVertex(Maths::Vector3(-1.0f, -1.0f, -1.0f)); // 0
        m_CubeHull->AddVertex(Maths::Vector3(-1.0f, 1.0f, -1.0f)); // 1
        m_CubeHull->AddVertex(Maths::Vector3(1.0f, 1.0f, -1.0f)); // 2
        m_CubeHull->AddVertex(Maths::Vector3(1.0f, -1.0f, -1.0f)); // 3

        m_CubeHull->AddVertex(Maths::Vector3(-1.0f, -1.0f, 1.0f)); // 4
        m_CubeHull->AddVertex(Maths::Vector3(-1.0f, 1.0f, 1.0f)); // 5
        m_CubeHull->AddVertex(Maths::Vector3(1.0f, 1.0f, 1.0f)); // 6
        m_CubeHull->AddVertex(Maths::Vector3(1.0f, -1.0f, 1.0f)); // 7

        int face1[] = { 0, 1, 2, 3 };
        int face2[] = { 7, 6, 5, 4 };
        int face3[] = { 5, 6, 2, 1 };
        int face4[] = { 0, 3, 7, 4 };
        int face5[] = { 6, 7, 3, 2 };
        int face6[] = { 4, 5, 1, 0 };

        //Faces
        m_CubeHull->AddFace(Maths::Vector3(0.0f, 0.0f, -1.0f), 4, face1);
        m_CubeHull->AddFace(Maths::Vector3(0.0f, 0.0f, 1.0f), 4, face2);
        m_CubeHull->AddFace(Maths::Vector3(0.0f, 1.0f, 0.0f), 4, face3);
        m_CubeHull->AddFace(Maths::Vector3(0.0f, -1.0f, 0.0f), 4, face4);
        m_CubeHull->AddFace(Maths::Vector3(1.0f, 0.0f, 0.0f), 4, face5);
        m_CubeHull->AddFace(Maths::Vector3(-1.0f, 0.0f, 0.0f), 4, face6);
    }
}
