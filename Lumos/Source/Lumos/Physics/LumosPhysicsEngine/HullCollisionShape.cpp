#include "Precompiled.h"
#include "HullCollisionShape.h"
#include "RigidBody3D.h"
#include "Maths/Matrix3.h"

#include "Graphics/Mesh.h"

namespace Lumos
{
    HullCollisionShape::HullCollisionShape()
    {
        m_HalfDimensions = Maths::Vector3(0.5f, 0.5f, 0.5f);
        m_Type = CollisionShapeType::CollisionHull;
        m_Axes.resize(3);
    }

    HullCollisionShape::~HullCollisionShape()
    {
    }

    void HullCollisionShape::BuildFromMesh(Graphics::Mesh* mesh)
    {
        m_Hull = CreateSharedRef<Hull>();

        auto vertexBuffer = mesh->GetVertexBuffer();
        Graphics::Vertex* vertices = vertexBuffer->GetPointer<Graphics::Vertex>();
        uint32_t size = vertexBuffer->GetSize();
        uint32_t count = size / sizeof(Graphics::Vertex);

        uint32_t* indices = mesh->GetIndexBuffer()->GetPointer<uint32_t>();
        uint32_t indexCount = mesh->GetIndexBuffer()->GetCount();

        for(size_t i = 0; i < count; i++)
        {
            m_Hull->AddVertex(vertices[i].Position);
        }

        for(size_t i = 0; i < indexCount; i += 3)
        {
            Maths::Vector3 n1 = vertices[indices[i]].Normal;
            Maths::Vector3 n2 = vertices[indices[i + 1]].Normal;
            Maths::Vector3 n3 = vertices[indices[i + 2]].Normal;
            Maths::Vector3 normal = n1 + n2 + n3;
            normal.Normalise();

            int vertexIdx[] = { (int)indices[i], (int)indices[i + 1], (int)indices[i + 2] };
            m_Hull->AddFace(normal, 3, vertexIdx);
        }

        m_Edges.resize(m_Hull->GetNumEdges());

        vertexBuffer->ReleasePointer();
        mesh->GetIndexBuffer()->ReleasePointer();
    }

    Maths::Matrix3 HullCollisionShape::BuildInverseInertia(float invMass) const
    {
        LUMOS_PROFILE_FUNCTION();
        Maths::Matrix3 inertia;

        Maths::Vector3 dimsSq = (m_HalfDimensions + m_HalfDimensions);
        dimsSq = dimsSq * dimsSq;

        inertia.m00_ = 12.f * invMass * 1.f / (dimsSq.y + dimsSq.z);
        inertia.m11_ = 12.f * invMass * 1.f / (dimsSq.x + dimsSq.z);
        inertia.m22_ = 12.f * invMass * 1.f / (dimsSq.x + dimsSq.y);

        return inertia;
    }

    std::vector<Maths::Vector3>& HullCollisionShape::GetCollisionAxes(const RigidBody3D* currentObject)
    {
        LUMOS_PROFILE_FUNCTION();
        {
            Maths::Matrix3 objOrientation = currentObject->GetOrientation().RotationMatrix();
            m_Axes[0] = (objOrientation * Maths::Vector3(1.0f, 0.0f, 0.0f)); //X - Axis
            m_Axes[1] = (objOrientation * Maths::Vector3(0.0f, 1.0f, 0.0f)); //Y - Axis
            m_Axes[2] = (objOrientation * Maths::Vector3(0.0f, 0.0f, 1.0f)); //Z - Axis
        }

        return m_Axes;
    }

    std::vector<CollisionEdge>& HullCollisionShape::GetEdges(const RigidBody3D* currentObject)
    {
        LUMOS_PROFILE_FUNCTION();
        {
            Maths::Matrix4 transform = currentObject->GetWorldSpaceTransform() * m_LocalTransform;
            for(unsigned int i = 0; i < m_Hull->GetNumEdges(); ++i)
            {
                const HullEdge& edge = m_Hull->GetEdge(i);
                Maths::Vector3 A = transform * m_Hull->GetVertex(edge.vStart).pos;
                Maths::Vector3 B = transform * m_Hull->GetVertex(edge.vEnd).pos;

                m_Edges[i] = { A, B };
            }
        }
        return m_Edges;
    }

    void HullCollisionShape::GetMinMaxVertexOnAxis(const RigidBody3D* currentObject, const Maths::Vector3& axis, Maths::Vector3* out_min, Maths::Vector3* out_max) const
    {
        LUMOS_PROFILE_FUNCTION();
        Maths::Matrix4 wsTransform = currentObject ? currentObject->GetWorldSpaceTransform() * m_LocalTransform : m_LocalTransform;
        const Maths::Vector3 local_axis = wsTransform.ToMatrix3().Transpose() * axis;

        int vMin, vMax;

        m_Hull->GetMinMaxVerticesInAxis(local_axis, &vMin, &vMax);

        if(out_min)
            *out_min = wsTransform * m_Hull->GetVertex(vMin).pos;
        if(out_max)
            *out_max = wsTransform * m_Hull->GetVertex(vMax).pos;
    }

    void HullCollisionShape::GetIncidentReferencePolygon(const RigidBody3D* currentObject,
        const Maths::Vector3& axis,
        ReferencePolygon& refPolygon) const
    {
        LUMOS_PROFILE_FUNCTION();
        Maths::Matrix4 wsTransform = currentObject ? currentObject->GetWorldSpaceTransform() * m_LocalTransform : m_LocalTransform;

        const Maths::Matrix3 invNormalMatrix = wsTransform.ToMatrix3().Inverse();
        const Maths::Matrix3 normalMatrix = invNormalMatrix.Transpose();

        const Maths::Vector3 local_axis = invNormalMatrix * axis;

        int minVertex, maxVertex;
        m_Hull->GetMinMaxVerticesInAxis(local_axis, &minVertex, &maxVertex);

        const HullVertex& vert = m_Hull->GetVertex(maxVertex);

        const HullFace* best_face = nullptr;
        float best_correlation = -FLT_MAX;
        for(int faceIdx : vert.enclosing_faces)
        {
            const HullFace* face = &m_Hull->GetFace(faceIdx);
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
                const HullVertex& vertex = m_Hull->GetVertex(vertIdx);
                refPolygon.Faces[refPolygon.FaceCount++] = wsTransform * vertex.pos;
            }
        }

        if(best_face)
        {
            //Add the reference face itself to the list of adjacent planes
            Maths::Vector3 wsPointOnPlane = wsTransform * m_Hull->GetVertex(m_Hull->GetEdge(best_face->edge_ids[0]).vStart).pos;
            Maths::Vector3 planeNrml = -(normalMatrix * best_face->normal);
            planeNrml.Normalise();
            float planeDist = -Maths::Vector3::Dot(planeNrml, wsPointOnPlane);

            refPolygon.AdjacentPlanes[refPolygon.PlaneCount++] = { planeNrml, planeDist };

            for(int edgeIdx : best_face->edge_ids)
            {
                const HullEdge& edge = m_Hull->GetEdge(edgeIdx);

                wsPointOnPlane = wsTransform * m_Hull->GetVertex(edge.vStart).pos;

                for(int adjFaceIdx : edge.enclosing_faces)
                {
                    if(adjFaceIdx != best_face->idx)
                    {
                        const HullFace& adjFace = m_Hull->GetFace(adjFaceIdx);

                        planeNrml = -(normalMatrix * adjFace.normal);
                        planeNrml.Normalise();
                        planeDist = -Maths::Vector3::Dot(planeNrml, wsPointOnPlane);

                        refPolygon.AdjacentPlanes[refPolygon.PlaneCount++] = { planeNrml, planeDist };
                    }
                }
            }
        }
    }

    void HullCollisionShape::DebugDraw(const RigidBody3D* currentObject) const
    {
        Maths::Matrix4 transform = currentObject->GetWorldSpaceTransform() * m_LocalTransform;
        m_Hull->DebugDraw(transform);
    }
}
