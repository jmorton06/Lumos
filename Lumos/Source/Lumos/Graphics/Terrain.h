#pragma once
#include "Mesh.h"
#include "Core/DataStructures/TDArray.h"

namespace Lumos
{
    class LUMOS_EXPORT Terrain : public Graphics::Mesh
    {
    public:
        // tileOriginX/Z are grid-unit offsets fed into the noise function so adjacent
        // tiles share edge values. Default 0 = a single isolated tile.
        Terrain(int width = 500, int height = 500, int lowside = 50, int lowscale = 10,
                float xRand = 1.0f, float yRand = 150.0f, float zRand = 1.0f,
                float texRandX = 1.0f / 16.0f, float texRandZ = 1.0f / 16.0f,
                int tileOriginX = 0, int tileOriginZ = 0);

        const TDArray<float>& GetHeightData() const { return m_HeightData; }
        int GetGridWidth() const { return m_GridWidth; }
        int GetGridHeight() const { return m_GridHeight; }
        float GetXScale() const { return m_XScale; }
        float GetYScale() const { return m_YScale; }
        float GetZScale() const { return m_ZScale; }
        int GetTileOriginX() const { return m_TileOriginX; }
        int GetTileOriginZ() const { return m_TileOriginZ; }

        // Rebuild geometry from externally supplied heights (e.g. sculpt edits
        // or .lhmap load). Grid dimensions and scales unchanged; vertex buffer
        // and bounding box are refreshed, normals recomputed. Pass nullptr to
        // rebuild from the current m_HeightData buffer (after the caller has
        // mutated it directly).
        void Rebuild(const float* heights = nullptr);

    private:
        TDArray<float> m_HeightData;
        int m_GridWidth   = 0;
        int m_GridHeight  = 0;
        float m_XScale    = 1.0f;
        float m_YScale    = 1.0f;
        float m_ZScale    = 1.0f;
        float m_TexRandX  = 1.0f / 16.0f;
        float m_TexRandZ  = 1.0f / 16.0f;
        int m_TileOriginX = 0;
        int m_TileOriginZ = 0;
    };
}
