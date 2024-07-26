#include "Precompiled.h"
#include "HullCollisionShape.h"
#include "Physics/LumosPhysicsEngine/RigidBody3D.h"
#include "Maths/Matrix3.h"
#include "Maths/MathsUtilities.h"
#include "Graphics/Mesh.h"
#include "Graphics/MeshFactory.h"

namespace Lumos
{
    HullCollisionShape::HullCollisionShape()
    {
        m_HalfDimensions = Vec3(1.0f);
        m_Type           = CollisionShapeType::CollisionHull;
        m_Axes.Resize(3);

        auto test = Lumos::SharedPtr<Lumos::Graphics::Mesh>(Lumos::Graphics::CreatePrimative(Lumos::Graphics::PrimitiveType::Cube));
        BuildFromMesh(test.get());

        m_LocalTransform = Mat4::Scale(m_HalfDimensions);
        m_Edges.Resize(m_Hull->GetNumEdges());
    }

    HullCollisionShape::~HullCollisionShape()
    {
    }

    void HullCollisionShape::BuildFromMesh(Graphics::Mesh* mesh)
    {
        m_Hull               = CreateSharedPtr<Hull>();
        const auto& vertices = mesh->GetVertices();
        const auto& indices  = mesh->GetIndices();

        /*     auto vertexBuffer          = mesh->GetVertexBuffer();
             Graphics::Vertex* vertices = vertexBuffer->GetPointer<Graphics::Vertex>();
             uint32_t size              = vertexBuffer->GetSize();
             uint32_t count             = size / sizeof(Graphics::Vertex);

             uint32_t* indices   = mesh->GetIndexBuffer()->GetPointer<uint32_t>();
             uint32_t indexCount = mesh->GetIndexBuffer()->GetCount();*/

        for(size_t i = 0; i < vertices.Size(); i++)
        {
            m_Hull->AddVertex(vertices[i].Position);
        }

        for(size_t i = 0; i < indices.Size(); i += 3)
        {
            Vec3 n1     = vertices[indices[i]].Normal;
            Vec3 n2     = vertices[indices[i + 1]].Normal;
            Vec3 n3     = vertices[indices[i + 2]].Normal;
            Vec3 normal = n1 + n2 + n3;
            normal.Normalise();

            int vertexIdx[] = { (int)indices[i], (int)indices[i + 1], (int)indices[i + 2] };
            m_Hull->AddFace(normal, 3, vertexIdx);
        }

        m_Edges.Resize(m_Hull->GetNumEdges());
    }

    // Mat3 HullCollisionShape::GetLocalInertiaTensor(float mass)
    // {
    //     Mat3 inertia;
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

    Mat3 HullCollisionShape::BuildInverseInertia(float invMass) const
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        Mat3 inertia(1.0f);

        Vec3 dimsSq = (m_HalfDimensions + m_HalfDimensions);
        dimsSq      = dimsSq * dimsSq;
        inertia.SetDiagonal(Vec3(12.f * invMass * 1.f / (dimsSq.y + dimsSq.z), 12.f * invMass * 1.f / (dimsSq.x + dimsSq.z), 12.f * invMass * 1.f / (dimsSq.x + dimsSq.y)));

        return inertia;
    }

    TDArray<Vec3>& HullCollisionShape::GetCollisionAxes(const RigidBody3D* currentObject)
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        {
            Mat3 objOrientation = Mat3(currentObject->GetOrientation());
            m_Axes[0]           = (objOrientation * Vec3(1.0f, 0.0f, 0.0f)); // X - Axis
            m_Axes[1]           = (objOrientation * Vec3(0.0f, 1.0f, 0.0f)); // Y - Axis
            m_Axes[2]           = (objOrientation * Vec3(0.0f, 0.0f, 1.0f)); // Z - Axis
        }

        return m_Axes;
    }

    TDArray<CollisionEdge>& HullCollisionShape::GetEdges(const RigidBody3D* currentObject)
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        {
            Mat4 transform = currentObject->GetWorldSpaceTransform() * m_LocalTransform;
            for(unsigned int i = 0; i < m_Hull->GetNumEdges(); ++i)
            {
                const HullEdge& edge = m_Hull->GetEdge(i);
                Vec3 A               = transform * Vec4(m_Hull->GetVertex(edge.vStart).pos, 1.0f);
                Vec3 B               = transform * Vec4(m_Hull->GetVertex(edge.vEnd).pos, 1.0f);

                m_Edges[i] = { A, B };
            }
        }
        return m_Edges;
    }

    void HullCollisionShape::GetMinMaxVertexOnAxis(const RigidBody3D* currentObject, const Vec3& axis, Vec3* out_min, Vec3* out_max) const
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        Mat4 wsTransform      = currentObject ? currentObject->GetWorldSpaceTransform() * m_LocalTransform : m_LocalTransform;
        const Vec3 local_axis = Maths::Transpose(wsTransform) * Vec4(axis, 1.0f);

        int vMin, vMax;
        if(!m_Hull)
        {
            if(out_min)
                *out_min = Vec3(0.0f);
            if(out_max)
                *out_max = Vec3(0.0f);

            return;
        }

        m_Hull->GetMinMaxVerticesInAxis(local_axis, &vMin, &vMax);

        if(out_min)
            *out_min = wsTransform * Vec4(m_Hull->GetVertex(vMin).pos, 1.0f);
        if(out_max)
            *out_max = wsTransform * Vec4(m_Hull->GetVertex(vMax).pos, 1.0f);
    }

    void HullCollisionShape::GetIncidentReferencePolygon(const RigidBody3D* currentObject,
                                                         const Vec3& axis,
                                                         ReferencePolygon& refPolygon) const
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        Mat4 wsTransform = currentObject ? currentObject->GetWorldSpaceTransform() * m_LocalTransform : m_LocalTransform;

        const Mat3 invNormalMatrix = Mat4::Inverse(wsTransform);
        const Mat3 normalMatrix    = Maths::Transpose(invNormalMatrix);

        const Vec3 local_axis = invNormalMatrix * axis;

        int minVertex, maxVertex;
        m_Hull->GetMinMaxVerticesInAxis(local_axis, &minVertex, &maxVertex);

        const HullVertex& vert = m_Hull->GetVertex(maxVertex);

        const HullFace* best_face = nullptr;
        float best_correlation    = -FLT_MAX;
        for(int faceIdx : vert.enclosing_faces)
        {
            const HullFace* face         = &m_Hull->GetFace(faceIdx);
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
                const HullVertex& vertex                 = m_Hull->GetVertex(vertIdx);
                refPolygon.Faces[refPolygon.FaceCount++] = wsTransform * Vec4(vertex.pos, 1.0f);
            }
        }

        if(best_face)
        {
            // Add the reference face itself to the list of adjacent planes
            Vec3 wsPointOnPlane = wsTransform * Vec4(m_Hull->GetVertex(m_Hull->GetEdge(best_face->edge_ids[0]).vStart).pos, 1.0f);
            Vec3 planeNrml      = -(normalMatrix * best_face->normal);
            planeNrml.Normalise();
            float planeDist = -Maths::Dot(planeNrml, wsPointOnPlane);

            refPolygon.AdjacentPlanes[refPolygon.PlaneCount++] = { planeNrml, planeDist };

            for(int edgeIdx : best_face->edge_ids)
            {
                const HullEdge& edge = m_Hull->GetEdge(edgeIdx);

                wsPointOnPlane = wsTransform * Vec4(m_Hull->GetVertex(edge.vStart).pos, 1.0f);

                for(int adjFaceIdx : edge.enclosing_faces)
                {
                    if(adjFaceIdx != best_face->idx)
                    {
                        const HullFace& adjFace = m_Hull->GetFace(adjFaceIdx);

                        planeNrml = -(normalMatrix * adjFace.normal);
                        planeNrml.Normalise();
                        planeDist = -Maths::Dot(planeNrml, wsPointOnPlane);

                        refPolygon.AdjacentPlanes[refPolygon.PlaneCount++] = { planeNrml, planeDist };
                    }
                }
            }
        }
    }

    void HullCollisionShape::DebugDraw(const RigidBody3D* currentObject) const
    {
        if(!m_Hull)
            return;
        Mat4 transform = currentObject->GetWorldSpaceTransform() * m_LocalTransform;
        m_Hull->DebugDraw(transform);
    }
}
