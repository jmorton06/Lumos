#include "Precompiled.h"
#include "CuboidCollisionShape.h"
#include "Physics/LumosPhysicsEngine/RigidBody3D.h"
#include "Maths/Matrix3.h"
#include "Maths/MathsUtilities.h"

namespace Lumos
{
    CuboidCollisionShape::CuboidCollisionShape()
    {
        m_CuboidHalfDimensions = Vec3(0.5f, 0.5f, 0.5f);
        m_Type                 = CollisionShapeType::CollisionCuboid;
        m_LocalTransform       = Mat4(1.0f); // Mat4::Scale(m_CuboidHalfDimensions);
        m_CubeHull             = CreateSharedPtr<BoundingBoxHull>();
        m_CubeHull->Set(-m_CuboidHalfDimensions, m_CuboidHalfDimensions);
        m_CubeHull->UpdateHull();
        m_Axes.Resize(3);
        m_Edges.Resize(m_CubeHull->GetNumEdges());
    }

    CuboidCollisionShape::CuboidCollisionShape(const Vec3& halfdims)
    {
        m_CuboidHalfDimensions = halfdims;
        m_LocalTransform       = Mat4(1.0f); // Mat4::Scale(halfdims);
        m_Type                 = CollisionShapeType::CollisionCuboid;

        m_CubeHull = CreateSharedPtr<BoundingBoxHull>();
        m_CubeHull->Set(-m_CuboidHalfDimensions, m_CuboidHalfDimensions);
        m_CubeHull->UpdateHull();

        m_Axes.Resize(3);
        m_Edges.Resize(m_CubeHull->GetNumEdges());
    }

    CuboidCollisionShape::~CuboidCollisionShape()
    {
    }

    Mat3 CuboidCollisionShape::BuildInverseInertia(float invMass) const
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        // https://en.wikipedia.org/wiki/List_of_moments_of_inertia
        Mat3 inertia(1.0f);

        Vec3 dimsSq = (m_CuboidHalfDimensions + m_CuboidHalfDimensions);
        dimsSq      = dimsSq * dimsSq;
        inertia.SetDiagonal(Vec3(12.f * invMass * 1.f / (dimsSq.y + dimsSq.z), 12.f * invMass * 1.f / (dimsSq.x + dimsSq.z), 12.f * invMass * 1.f / (dimsSq.x + dimsSq.y)));

        return inertia;
    }

    TDArray<Vec3>& CuboidCollisionShape::GetCollisionAxes(const RigidBody3D* currentObject)
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        {
            m_Axes.Resize(3);

            Mat3 objOrientation = Mat3(currentObject->GetOrientation());     //.RotationMatrix();
            m_Axes[0]           = (objOrientation * Vec3(1.0f, 0.0f, 0.0f)); // X - Axis
            m_Axes[1]           = (objOrientation * Vec3(0.0f, 1.0f, 0.0f)); // Y - Axis
            m_Axes[2]           = (objOrientation * Vec3(0.0f, 0.0f, 1.0f)); // Z - Axis
        }

        return m_Axes;
    }

    TDArray<CollisionEdge>& CuboidCollisionShape::GetEdges(const RigidBody3D* currentObject)
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        {
            Mat4 transform = currentObject->GetWorldSpaceTransform() * m_LocalTransform;
            for(unsigned int i = 0; i < m_CubeHull->GetNumEdges(); ++i)
            {
                const HullEdge& edge = m_CubeHull->GetEdge(i);
                Vec3 A               = transform * Vec4(m_CubeHull->GetVertex(edge.vStart).pos, 1.0f);
                Vec3 B               = transform * Vec4(m_CubeHull->GetVertex(edge.vEnd).pos, 1.0f);

                m_Edges[i] = { A, B };
            }
        }
        return m_Edges;
    }

    void CuboidCollisionShape::GetMinMaxVertexOnAxis(const RigidBody3D* currentObject, const Vec3& axis, Vec3* out_min, Vec3* out_max) const
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        Mat4 wsTransform      = currentObject ? currentObject->GetWorldSpaceTransform() * m_LocalTransform : m_LocalTransform;
        const Vec3 local_axis = Maths::Transpose(Mat3(wsTransform)) * axis;

        int vMin, vMax;

        m_CubeHull->GetMinMaxVerticesInAxis(local_axis, &vMin, &vMax);

        if(out_min)
            *out_min = wsTransform * Vec4(m_CubeHull->GetVertex(vMin).pos, 1.0f);
        if(out_max)
            *out_max = wsTransform * Vec4(m_CubeHull->GetVertex(vMax).pos, 1.0f);
    }

    void CuboidCollisionShape::GetIncidentReferencePolygon(const RigidBody3D* currentObject,
                                                           const Vec3& axis,
                                                           ReferencePolygon& refPolygon) const
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        Mat4 wsTransform = currentObject ? currentObject->GetWorldSpaceTransform() * m_LocalTransform : m_LocalTransform;

        const Mat3 invNormalMatrix = Maths::Transpose(Mat3(wsTransform));
        const Mat3 normalMatrix    = invNormalMatrix.Inverse();

        const Vec3 local_axis = invNormalMatrix * axis;

        int minVertex, maxVertex;
        m_CubeHull->GetMinMaxVerticesInAxis(local_axis, &minVertex, &maxVertex);

        const HullVertex& vert = m_CubeHull->GetVertex(maxVertex);

        const HullFace* best_face = nullptr;
        float best_correlation    = -FLT_MAX;
        for(int faceIdx : vert.enclosing_faces)
        {
            const HullFace* face         = &m_CubeHull->GetFace(faceIdx);
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
                const HullVertex& vertex                 = m_CubeHull->GetVertex(vertIdx);
                refPolygon.Faces[refPolygon.FaceCount++] = wsTransform * Vec4(vertex.pos, 1.0f);
            }
        }

        if(best_face)
        {
            // Add the reference face itself to the list of adjacent planes
            Vec3 wsPointOnPlane = wsTransform * Vec4(m_CubeHull->GetVertex(m_CubeHull->GetEdge(best_face->edge_ids[0]).vStart).pos, 1.0f);
            Vec3 planeNrml      = -(normalMatrix * best_face->normal);
            planeNrml.Normalise();
            float planeDist = -Maths::Dot(planeNrml, wsPointOnPlane);

            refPolygon.AdjacentPlanes[refPolygon.PlaneCount++] = Plane(planeNrml, planeDist);

            for(int edgeIdx : best_face->edge_ids)
            {
                const HullEdge& edge = m_CubeHull->GetEdge(edgeIdx);

                wsPointOnPlane = wsTransform * Vec4(m_CubeHull->GetVertex(edge.vStart).pos, 1.0f);

                for(int adjFaceIdx : edge.enclosing_faces)
                {
                    if(adjFaceIdx != best_face->idx)
                    {
                        const HullFace& adjFace = m_CubeHull->GetFace(adjFaceIdx);

                        planeNrml = -(normalMatrix * adjFace.normal);
                        planeNrml.Normalise();
                        planeDist = -Maths::Dot(planeNrml, wsPointOnPlane);

                        refPolygon.AdjacentPlanes[refPolygon.PlaneCount++] = Plane(planeNrml, planeDist);
                    }
                }
            }
        }
    }

    void CuboidCollisionShape::DebugDraw(const RigidBody3D* currentObject) const
    {
        Mat4 transform = currentObject->GetWorldSpaceTransform() * m_LocalTransform;

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
        // m_CubeHull->AddVertex(Vec3(-1.0f, -1.0f, -1.0f)); // 0
        // m_CubeHull->AddVertex(Vec3(-1.0f, 1.0f, -1.0f));  // 1
        // m_CubeHull->AddVertex(Vec3(1.0f, 1.0f, -1.0f));   // 2
        // m_CubeHull->AddVertex(Vec3(1.0f, -1.0f, -1.0f));  // 3

        // m_CubeHull->AddVertex(Vec3(-1.0f, -1.0f, 1.0f)); // 4
        // m_CubeHull->AddVertex(Vec3(-1.0f, 1.0f, 1.0f));  // 5
        // m_CubeHull->AddVertex(Vec3(1.0f, 1.0f, 1.0f));   // 6
        // m_CubeHull->AddVertex(Vec3(1.0f, -1.0f, 1.0f));  // 7

        // int face1[] = { 0, 1, 2, 3 };
        // int face2[] = { 7, 6, 5, 4 };
        // int face3[] = { 5, 6, 2, 1 };
        // int face4[] = { 0, 3, 7, 4 };
        // int face5[] = { 6, 7, 3, 2 };
        // int face6[] = { 4, 5, 1, 0 };

        //// Faces
        // m_CubeHull->AddFace(Vec3(0.0f, 0.0f, -1.0f), 4, face1);
        // m_CubeHull->AddFace(Vec3(0.0f, 0.0f, 1.0f), 4, face2);
        // m_CubeHull->AddFace(Vec3(0.0f, 1.0f, 0.0f), 4, face3);
        // m_CubeHull->AddFace(Vec3(0.0f, -1.0f, 0.0f), 4, face4);
        // m_CubeHull->AddFace(Vec3(1.0f, 0.0f, 0.0f), 4, face5);
        // m_CubeHull->AddFace(Vec3(-1.0f, 0.0f, 0.0f), 4, face6);
    }
}
