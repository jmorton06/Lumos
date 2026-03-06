#pragma once
#include "CollisionShape.h"

namespace Lumos
{
    namespace Graphics
    {
        class Mesh;
    }

    struct MeshTriangle
    {
        Vec3 v0, v1, v2;
        Vec3 Normal;
    };

    class LUMOS_EXPORT MeshCollisionShape : public CollisionShape
    {
    public:
        MeshCollisionShape();
        MeshCollisionShape(const TDArray<Vec3>& vertices, const TDArray<uint32_t>& indices);
        ~MeshCollisionShape();

        void SetMeshData(const TDArray<Vec3>& vertices, const TDArray<uint32_t>& indices);
        void BuildFromMesh(Graphics::Mesh* mesh);

        uint32_t GetTriangleCount() const { return (uint32_t)m_Triangles.Size(); }
        const MeshTriangle& GetTriangle(uint32_t index) const { return m_Triangles[index]; }

        Mat3 BuildInverseInertia(float invMass) const override;
        float GetSize() const override;
        void DebugDraw(const RigidBody3D* currentObject) const override;

        TDArray<Vec3>& GetCollisionAxes(const RigidBody3D* currentObject) override;
        TDArray<CollisionEdge>& GetEdges(const RigidBody3D* currentObject) override;
        void GetMinMaxVertexOnAxis(const RigidBody3D* currentObject, const Vec3& axis, Vec3* out_min, Vec3* out_max) const override;
        void GetIncidentReferencePolygon(const RigidBody3D* currentObject, const Vec3& axis, ReferencePolygon& refPolygon) const override;

        // Mesh-specific collision queries
        bool RayIntersect(const Vec3& origin, const Vec3& direction, float maxDist, float& outT, Vec3& outNormal) const;

    protected:
        void ComputeBounds();

        TDArray<MeshTriangle> m_Triangles;
        TDArray<Vec3> m_Vertices;
        Vec3 m_BoundsMin;
        Vec3 m_BoundsMax;
    };
}
