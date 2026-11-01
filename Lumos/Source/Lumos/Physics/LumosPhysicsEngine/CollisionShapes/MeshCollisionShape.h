#pragma once
#include "CollisionShape.h"
#include "Scene/Serialisation/Serialisation.h"

namespace Lumos
{
    namespace Graphics
    {
        class Mesh;
        class Model;
    }

    struct MeshTriangle
    {
        Vec3 v0, v1, v2;
        Vec3 Normal;
    };

    // Leaf when Count > 0 (triangles First .. First + Count), otherwise children are First and First + 1.
    struct MeshBVHNode
    {
        Vec3 Min;
        Vec3 Max;
        uint32_t First;
        uint32_t Count;
    };

    // Static triangle soup. Triangles are in the body's local space with any scale baked in.
    class LUMOS_EXPORT MeshCollisionShape : public CollisionShape
    {
    public:
        MeshCollisionShape();
        MeshCollisionShape(const TDArray<Vec3>& vertices, const TDArray<uint32_t>& indices);
        ~MeshCollisionShape();

        void SetMeshData(const TDArray<Vec3>& vertices, const TDArray<uint32_t>& indices);
        void BuildFromMesh(Graphics::Mesh* mesh);
        // Every mesh of the model, each vertex moved by `bake` (the model's scale relative to the body).
        bool BuildFromModel(Graphics::Model* model, const Mat4& bake);
        bool IsBuilt() const { return !m_Nodes.Empty(); }
        // share of the model's triangles kept (simplified) when built from the model, 0.01..1
        float GetDetail() const { return m_Detail; }
        void SetDetail(float detail) { m_Detail = detail < 0.01f ? 0.01f : (detail > 1.0f ? 1.0f : detail); }
        // drop the triangles: the physics engine builds them again from the model (and rewrites the .lcol)
        void RequestRebuild()
        {
            m_Triangles.Clear();
            m_Nodes.Clear();
            m_LoadFile = false;
        }

        // Triangles whose bounds touch the box (local space). Returns how many were written.
        uint32_t QueryAABB(const Vec3& boxMin, const Vec3& boxMax, uint32_t* outTriangles, uint32_t maxTriangles) const;
        // Nearest hit of a ray, or of a sphere of `radius` moved along it (local space, unit direction).
        bool RayCast(const Vec3& origin, const Vec3& direction, float maxDist, float radius, float& outT, Vec3& outNormal) const;

        // .lcol: the triangles and their BVH, so nothing is rebuilt on load. VFS paths.
        bool SaveToFile(const String8& path) const;
        bool LoadFromFile(const String8& path);

        // Where the baked mesh lives. Empty = always build from the entity's model.
        void SetFilePath(const char* path);
        const char* GetFilePath() const { return m_FilePath; }
        // false after a load failed or a rebake was asked for: build from the model and write the file
        bool ShouldLoadFile() const { return m_LoadFile && m_FilePath[0] != 0; }
        void SetShouldLoadFile(bool load) { m_LoadFile = load; }

        template <typename Archive>
        void save(Archive& archive) const
        {
            archive(std::string(m_FilePath), m_Detail);
        }

        template <typename Archive>
        void load(Archive& archive)
        {
            std::string path;
            archive(path);
            if(Serialisation::CurrentSceneVersion > 38)
                archive(m_Detail);
            SetFilePath(path.c_str());
            m_Type = CollisionShapeType::CollisionMesh;
            // triangles come from the file, or from the entity's model, once the physics engine sees the body
        }

        const Vec3& GetBoundsMin() const { return m_BoundsMin; }
        const Vec3& GetBoundsMax() const { return m_BoundsMax; }

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
        void BuildBVH();

        TDArray<MeshTriangle> m_Triangles;
        TDArray<MeshBVHNode> m_Nodes;
        TDArray<Vec3> m_Vertices;
        Vec3 m_BoundsMin;
        Vec3 m_BoundsMax;
        char m_FilePath[256] = { 0 };
        bool m_LoadFile      = true;
        float m_Detail       = 1.0f;
    };
}
