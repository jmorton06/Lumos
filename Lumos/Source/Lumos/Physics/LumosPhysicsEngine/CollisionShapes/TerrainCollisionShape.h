#pragma once
#include "CollisionShape.h"

namespace Lumos
{
    class LUMOS_EXPORT TerrainCollisionShape : public CollisionShape
    {
    public:
        TerrainCollisionShape();
        TerrainCollisionShape(uint32_t width, uint32_t height, float* heightData, float scaleXZ, float scaleY);
        ~TerrainCollisionShape();

        void SetHeightData(uint32_t width, uint32_t height, float* heightData, float scaleXZ, float scaleY);

        float GetHeightAt(float x, float z) const;
        Vec3 GetNormalAt(float x, float z) const;

        uint32_t GetWidth() const { return m_Width; }
        uint32_t GetHeight() const { return m_Height; }
        float GetScaleXZ() const { return m_ScaleXZ; }
        float GetScaleY() const { return m_ScaleY; }
        bool HasHeightData() const { return !m_HeightData.Empty(); }

        Mat3 BuildInverseInertia(float invMass) const override;
        float GetSize() const override;
        void DebugDraw(const RigidBody3D* currentObject) const override;

        TDArray<Vec3>& GetCollisionAxes(const RigidBody3D* currentObject) override;
        TDArray<CollisionEdge>& GetEdges(const RigidBody3D* currentObject) override;
        void GetMinMaxVertexOnAxis(const RigidBody3D* currentObject, const Vec3& axis, Vec3* out_min, Vec3* out_max) const override;
        void GetIncidentReferencePolygon(const RigidBody3D* currentObject, const Vec3& axis, ReferencePolygon& refPolygon) const override;

        // Terrain-specific collision query
        bool GetContactPoint(const Vec3& point, float radius, Vec3& outContact, Vec3& outNormal, float& outPenetration) const;

        template <typename Archive>
        void save(Archive& archive) const
        {
            archive(m_Width, m_Height, m_ScaleXZ, m_ScaleY);
        }

        template <typename Archive>
        void load(Archive& archive)
        {
            archive(m_Width, m_Height, m_ScaleXZ, m_ScaleY);
            m_Type = CollisionShapeType::CollisionTerrain;
            // Height data is re-populated by auto-attach from Terrain mesh
        }

    protected:
        TDArray<float> m_HeightData;
        uint32_t m_Width  = 0;
        uint32_t m_Height = 0;
        float m_ScaleXZ   = 1.0f;
        float m_ScaleY    = 1.0f;
        float m_MinHeight = 0.0f;
        float m_MaxHeight = 0.0f;
    };
}
