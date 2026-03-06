#include "Precompiled.h"
#include "MeshCollisionShape.h"
#include "Physics/LumosPhysicsEngine/RigidBody3D.h"
#include "Graphics/Renderers/DebugRenderer.h"
#include "Graphics/Mesh.h"
#include "Maths/MathsUtilities.h"

namespace Lumos
{
    MeshCollisionShape::MeshCollisionShape()
        : m_BoundsMin(0.0f)
        , m_BoundsMax(0.0f)
    {
        m_Type = CollisionShapeType::CollisionMesh;
    }

    MeshCollisionShape::MeshCollisionShape(const TDArray<Vec3>& vertices, const TDArray<uint32_t>& indices)
        : m_BoundsMin(0.0f)
        , m_BoundsMax(0.0f)
    {
        m_Type = CollisionShapeType::CollisionMesh;
        SetMeshData(vertices, indices);
    }

    MeshCollisionShape::~MeshCollisionShape()
    {
    }

    void MeshCollisionShape::SetMeshData(const TDArray<Vec3>& vertices, const TDArray<uint32_t>& indices)
    {
        m_Vertices = vertices;
        m_Triangles.Clear();

        // Build triangles from indices
        for(uint32_t i = 0; i + 2 < indices.Size(); i += 3)
        {
            MeshTriangle tri;
            tri.v0 = vertices[indices[i]];
            tri.v1 = vertices[indices[i + 1]];
            tri.v2 = vertices[indices[i + 2]];

            Vec3 edge1  = tri.v1 - tri.v0;
            Vec3 edge2  = tri.v2 - tri.v0;
            tri.Normal = Maths::Cross(edge1, edge2).Normalised();

            m_Triangles.PushBack(tri);
        }

        ComputeBounds();
    }

    void MeshCollisionShape::BuildFromMesh(Graphics::Mesh* mesh)
    {
        if(!mesh)
            return;

        // Get vertex and index data from mesh
        // This is a simplified version - actual implementation depends on Mesh class structure
        // For now, we assume the mesh has GetVertices() and GetIndices() methods

        // TODO: Implement based on actual Mesh class API
        LWARN("MeshCollisionShape::BuildFromMesh not fully implemented");
    }

    void MeshCollisionShape::ComputeBounds()
    {
        if(m_Vertices.Empty())
        {
            m_BoundsMin = Vec3(0.0f);
            m_BoundsMax = Vec3(0.0f);
            return;
        }

        m_BoundsMin = m_Vertices[0];
        m_BoundsMax = m_Vertices[0];

        for(const auto& v : m_Vertices)
        {
            m_BoundsMin = Maths::Min(m_BoundsMin, v);
            m_BoundsMax = Maths::Max(m_BoundsMax, v);
        }
    }

    Mat3 MeshCollisionShape::BuildInverseInertia(float invMass) const
    {
        // Approximate using bounding box inertia
        Vec3 size = m_BoundsMax - m_BoundsMin;
        float mass = 1.0f / invMass;
        float factor = mass / 12.0f;

        Mat3 inertia(1.0f);
        inertia.SetDiagonal(Vec3(
            1.0f / (factor * (size.y * size.y + size.z * size.z)),
            1.0f / (factor * (size.x * size.x + size.z * size.z)),
            1.0f / (factor * (size.x * size.x + size.y * size.y))));

        return inertia;
    }

    float MeshCollisionShape::GetSize() const
    {
        return Maths::Length(m_BoundsMax - m_BoundsMin);
    }

    void MeshCollisionShape::DebugDraw(const RigidBody3D* currentObject) const
    {
        Mat4 transform = currentObject ? currentObject->GetWorldSpaceTransform() : Mat4(1.0f);

        // Draw triangles (sparse for performance)
        uint32_t step = Maths::Max(1u, (uint32_t)m_Triangles.Size() / 100);

        for(uint32_t i = 0; i < m_Triangles.Size(); i += step)
        {
            const auto& tri = m_Triangles[i];
            Vec3 v0 = Vec3(transform * Vec4(tri.v0, 1.0f));
            Vec3 v1 = Vec3(transform * Vec4(tri.v1, 1.0f));
            Vec3 v2 = Vec3(transform * Vec4(tri.v2, 1.0f));

            DebugRenderer::DrawThickLine(v0, v1, 0.005f, false, Vec4(0.8f, 0.4f, 0.4f, 1.0f));
            DebugRenderer::DrawThickLine(v1, v2, 0.005f, false, Vec4(0.8f, 0.4f, 0.4f, 1.0f));
            DebugRenderer::DrawThickLine(v2, v0, 0.005f, false, Vec4(0.8f, 0.4f, 0.4f, 1.0f));
        }
    }

    TDArray<Vec3>& MeshCollisionShape::GetCollisionAxes(const RigidBody3D* currentObject)
    {
        m_Axes.Clear();

        // Add unique face normals (limited for performance)
        uint32_t maxAxes = 32;

        for(const auto& tri : m_Triangles)
        {
            if(m_Axes.Size() >= maxAxes)
                break;

            bool isDuplicate = false;
            for(const auto& axis : m_Axes)
            {
                if(Maths::Abs(Maths::Dot(axis, tri.Normal)) > 0.999f)
                {
                    isDuplicate = true;
                    break;
                }
            }

            if(!isDuplicate)
                m_Axes.PushBack(tri.Normal);
        }

        return m_Axes;
    }

    TDArray<CollisionEdge>& MeshCollisionShape::GetEdges(const RigidBody3D* currentObject)
    {
        m_Edges.Clear();

        // Return edges (limited for performance)
        uint32_t maxEdges = 64;

        for(const auto& tri : m_Triangles)
        {
            if(m_Edges.Size() >= maxEdges)
                break;

            m_Edges.PushBack(CollisionEdge(tri.v0, tri.v1));
            m_Edges.PushBack(CollisionEdge(tri.v1, tri.v2));
            m_Edges.PushBack(CollisionEdge(tri.v2, tri.v0));
        }

        return m_Edges;
    }

    void MeshCollisionShape::GetMinMaxVertexOnAxis(const RigidBody3D* currentObject, const Vec3& axis, Vec3* out_min, Vec3* out_max) const
    {
        if(m_Vertices.Empty())
        {
            if(out_min) *out_min = Vec3(0.0f);
            if(out_max) *out_max = Vec3(0.0f);
            return;
        }

        Mat4 transform = currentObject ? currentObject->GetWorldSpaceTransform() : Mat4(1.0f);

        float minProj = 1e30f;
        float maxProj = -1e30f;
        Vec3 minVert, maxVert;

        for(const auto& v : m_Vertices)
        {
            Vec3 worldV = Vec3(transform * Vec4(v, 1.0f));
            float proj = Maths::Dot(worldV, axis);

            if(proj < minProj) { minProj = proj; minVert = worldV; }
            if(proj > maxProj) { maxProj = proj; maxVert = worldV; }
        }

        if(out_min) *out_min = minVert;
        if(out_max) *out_max = maxVert;
    }

    void MeshCollisionShape::GetIncidentReferencePolygon(const RigidBody3D* currentObject, const Vec3& axis, ReferencePolygon& refPolygon) const
    {
        if(m_Triangles.Empty())
        {
            refPolygon.FaceCount = 0;
            return;
        }

        Mat4 transform = currentObject ? currentObject->GetWorldSpaceTransform() : Mat4(1.0f);
        Mat3 normalTransform = Mat3(transform);

        // Find triangle most aligned with axis
        float bestDot = -1e30f;
        int bestTriIdx = 0;

        for(uint32_t i = 0; i < m_Triangles.Size(); i++)
        {
            Vec3 worldNormal = (normalTransform * m_Triangles[i].Normal).Normalised();
            float dot = Maths::Dot(worldNormal, axis);

            if(dot > bestDot)
            {
                bestDot = dot;
                bestTriIdx = (int)i;
            }
        }

        const auto& tri = m_Triangles[bestTriIdx];
        refPolygon.Faces[0] = Vec3(transform * Vec4(tri.v0, 1.0f));
        refPolygon.Faces[1] = Vec3(transform * Vec4(tri.v1, 1.0f));
        refPolygon.Faces[2] = Vec3(transform * Vec4(tri.v2, 1.0f));
        refPolygon.FaceCount = 3;
        refPolygon.Normal = (normalTransform * tri.Normal).Normalised();
    }

    bool MeshCollisionShape::RayIntersect(const Vec3& origin, const Vec3& direction, float maxDist, float& outT, Vec3& outNormal) const
    {
        bool hit    = false;
        float bestT = maxDist;

        for(const auto& tri : m_Triangles)
        {
            // Moller-Trumbore intersection
            Vec3 edge1 = tri.v1 - tri.v0;
            Vec3 edge2 = tri.v2 - tri.v0;
            Vec3 h     = Maths::Cross(direction, edge2);
            float a    = Maths::Dot(edge1, h);

            if(Maths::Abs(a) < Maths::M_EPSILON)
                continue;

            float f = 1.0f / a;
            Vec3 s  = origin - tri.v0;
            float u = f * Maths::Dot(s, h);

            if(u < 0.0f || u > 1.0f)
                continue;

            Vec3 q  = Maths::Cross(s, edge1);
            float v = f * Maths::Dot(direction, q);

            if(v < 0.0f || u + v > 1.0f)
                continue;

            float t = f * Maths::Dot(edge2, q);

            if(t > Maths::M_EPSILON && t < bestT)
            {
                bestT     = t;
                outT      = t;
                outNormal = tri.Normal;
                hit       = true;
            }
        }

        return hit;
    }
}
