#include "Precompiled.h"
#include "TerrainCollisionShape.h"
#include "Physics/LumosPhysicsEngine/RigidBody3D.h"
#include "Graphics/Renderers/DebugRenderer.h"
#include "Maths/MathsUtilities.h"

namespace Lumos
{
    TerrainCollisionShape::TerrainCollisionShape()
    {
        m_Type = CollisionShapeType::CollisionTerrain;
    }

    TerrainCollisionShape::TerrainCollisionShape(uint32_t width, uint32_t height, float* heightData, float scaleXZ, float scaleY)
    {
        m_Type = CollisionShapeType::CollisionTerrain;
        SetHeightData(width, height, heightData, scaleXZ, scaleY);
    }

    TerrainCollisionShape::~TerrainCollisionShape()
    {
    }

    void TerrainCollisionShape::SetHeightData(uint32_t width, uint32_t height, float* heightData, float scaleXZ, float scaleY)
    {
        m_Width   = width;
        m_Height  = height;
        m_ScaleXZ = scaleXZ;
        m_ScaleY  = scaleY;

        uint32_t dataSize = width * height;
        m_HeightData.Resize(dataSize);

        m_MinHeight = heightData[0];
        m_MaxHeight = heightData[0];

        for(uint32_t i = 0; i < dataSize; i++)
        {
            m_HeightData[i] = heightData[i];
            m_MinHeight = Maths::Min(m_MinHeight, heightData[i]);
            m_MaxHeight = Maths::Max(m_MaxHeight, heightData[i]);
        }
    }

    float TerrainCollisionShape::GetHeightAt(float x, float z) const
    {
        if(m_HeightData.Empty() || m_Width < 2 || m_Height < 2)
            return 0.0f;

        // Convert world coords to heightfield coords
        float gridX = x / m_ScaleXZ;
        float gridZ = z / m_ScaleXZ;

        // Clamp to valid range
        gridX = Maths::Clamp(gridX, 0.0f, (float)(m_Width - 1));
        gridZ = Maths::Clamp(gridZ, 0.0f, (float)(m_Height - 1));

        int x0 = (int)gridX;
        int z0 = (int)gridZ;
        int x1 = Maths::Min(x0 + 1, (int)m_Width - 1);
        int z1 = Maths::Min(z0 + 1, (int)m_Height - 1);

        float fx = gridX - x0;
        float fz = gridZ - z0;

        // Bilinear interpolation (terrain stores data as [x * width + z])
        float h00 = m_HeightData[x0 * m_Width + z0];
        float h10 = m_HeightData[x1 * m_Width + z0];
        float h01 = m_HeightData[x0 * m_Width + z1];
        float h11 = m_HeightData[x1 * m_Width + z1];

        float h0 = h00 * (1.0f - fx) + h10 * fx;
        float h1 = h01 * (1.0f - fx) + h11 * fx;

        return (h0 * (1.0f - fz) + h1 * fz) * m_ScaleY;
    }

    Vec3 TerrainCollisionShape::GetNormalAt(float x, float z) const
    {
        float epsilon = m_ScaleXZ * 0.5f;

        float hL = GetHeightAt(x - epsilon, z);
        float hR = GetHeightAt(x + epsilon, z);
        float hD = GetHeightAt(x, z - epsilon);
        float hU = GetHeightAt(x, z + epsilon);

        Vec3 normal(hL - hR, 2.0f * epsilon, hD - hU);
        return normal.Normalised();
    }

    bool TerrainCollisionShape::GetContactPoint(const Vec3& point, float radius, Vec3& outContact, Vec3& outNormal, float& outPenetration) const
    {
        float terrainHeight = GetHeightAt(point.x, point.z);
        float objectBottom  = point.y - radius;

        if(objectBottom < terrainHeight)
        {
            outNormal      = GetNormalAt(point.x, point.z);
            outPenetration = terrainHeight - objectBottom;
            outContact     = Vec3(point.x, terrainHeight, point.z);
            return true;
        }

        return false;
    }

    Mat3 TerrainCollisionShape::BuildInverseInertia(float invMass) const
    {
        // Terrain is static, infinite inertia
        return Mat3(0.0f);
    }

    float TerrainCollisionShape::GetSize() const
    {
        return Maths::Max(m_Width * m_ScaleXZ, m_Height * m_ScaleXZ);
    }

    void TerrainCollisionShape::DebugDraw(const RigidBody3D* currentObject) const
    {
        // Draw grid outline
        if(m_Width < 2 || m_Height < 2 || m_HeightData.Empty())
            return;

        Vec3 pos = currentObject ? currentObject->GetPosition() : Vec3(0.0f);

        // Draw sparse grid for performance
        uint32_t step = Maths::Max(1u, m_Width / 32);

        for(uint32_t z = 0; z < m_Height - step; z += step)
        {
            for(uint32_t x = 0; x < m_Width - step; x += step)
            {
                float worldX = x * m_ScaleXZ + pos.x;
                float worldZ = z * m_ScaleXZ + pos.z;
                float h      = m_HeightData[x * m_Width + z] * m_ScaleY + pos.y;

                float worldX2 = (x + step) * m_ScaleXZ + pos.x;
                float worldZ2 = (z + step) * m_ScaleXZ + pos.z;
                float h2      = m_HeightData[(x + step) * m_Width + z] * m_ScaleY + pos.y;
                float h3      = m_HeightData[x * m_Width + (z + step)] * m_ScaleY + pos.y;

                DebugRenderer::DrawThickLine(Vec3(worldX, h, worldZ), Vec3(worldX2, h2, worldZ), 0.01f, false, Vec4(0.5f, 0.8f, 0.5f, 1.0f));
                DebugRenderer::DrawThickLine(Vec3(worldX, h, worldZ), Vec3(worldX, h3, worldZ2), 0.01f, false, Vec4(0.5f, 0.8f, 0.5f, 1.0f));
            }
        }
    }

    TDArray<Vec3>& TerrainCollisionShape::GetCollisionAxes(const RigidBody3D* currentObject)
    {
        m_Axes.Clear();
        m_Axes.PushBack(Vec3(0.0f, 1.0f, 0.0f)); // Up vector
        return m_Axes;
    }

    TDArray<CollisionEdge>& TerrainCollisionShape::GetEdges(const RigidBody3D* currentObject)
    {
        m_Edges.Clear();
        return m_Edges;
    }

    void TerrainCollisionShape::GetMinMaxVertexOnAxis(const RigidBody3D* currentObject, const Vec3& axis, Vec3* out_min, Vec3* out_max) const
    {
        Vec3 pos = currentObject ? currentObject->GetPosition() : Vec3(0.0f);

        float minProj = 1e30f;
        float maxProj = -1e30f;
        Vec3 minVert, maxVert;

        // 8 AABB corners with proper min/max heights
        float maxX = (m_Width - 1) * m_ScaleXZ;
        float maxZ = (m_Height - 1) * m_ScaleXZ;
        float minY = m_MinHeight * m_ScaleY;
        float maxY = m_MaxHeight * m_ScaleY;

        Vec3 corners[8] = {
            pos + Vec3(0,    minY, 0),
            pos + Vec3(maxX, minY, 0),
            pos + Vec3(0,    minY, maxZ),
            pos + Vec3(maxX, minY, maxZ),
            pos + Vec3(0,    maxY, 0),
            pos + Vec3(maxX, maxY, 0),
            pos + Vec3(0,    maxY, maxZ),
            pos + Vec3(maxX, maxY, maxZ)
        };

        for(int i = 0; i < 8; i++)
        {
            float proj = Maths::Dot(corners[i], axis);
            if(proj < minProj) { minProj = proj; minVert = corners[i]; }
            if(proj > maxProj) { maxProj = proj; maxVert = corners[i]; }
        }

        if(out_min) *out_min = minVert;
        if(out_max) *out_max = maxVert;
    }

    void TerrainCollisionShape::GetIncidentReferencePolygon(const RigidBody3D* currentObject, const Vec3& axis, ReferencePolygon& refPolygon) const
    {
        // Return a single contact point — the terrain surface point under the collision
        // This feeds into BuildCollisionManifold's FaceCount == 1 path
        refPolygon.FaceCount = 1;
        refPolygon.Normal    = Vec3(0.0f, 1.0f, 0.0f);

        // Use axis to find a representative point: project axis onto XZ to locate the contact
        Vec3 pos = currentObject ? currentObject->GetPosition() : Vec3(0.0f);

        // The axis points toward the collision — sample terrain at the XZ implied by axis
        // For terrain, we use the midpoint of the terrain as fallback
        float sampleX = m_Width * m_ScaleXZ * 0.5f;
        float sampleZ = m_Height * m_ScaleXZ * 0.5f;

        // If axis has a meaningful XZ component, offset from center
        if(Maths::Abs(axis.x) > Maths::M_EPSILON || Maths::Abs(axis.z) > Maths::M_EPSILON)
        {
            // Scale from normalized axis into terrain space
            sampleX += axis.x * m_Width * m_ScaleXZ * 0.25f;
            sampleZ += axis.z * m_Height * m_ScaleXZ * 0.25f;
        }

        float h = GetHeightAt(sampleX, sampleZ);
        refPolygon.Faces[0] = pos + Vec3(sampleX, h, sampleZ);
        refPolygon.Normal    = GetNormalAt(sampleX, sampleZ);
    }
}
