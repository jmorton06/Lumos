#include "Precompiled.h"
#include "Terrain.h"
#include "Maths/BoundingBox.h"
#include "Graphics/RHI/IndexBuffer.h"
#include "Graphics/RHI/VertexBuffer.h"
#include <stb/stb_perlin.h>

namespace Lumos
{
    float Noise(int x, int y)
    {
        const int offsetx  = 100;
        const int offsety  = -50;
        const float layer1 = 25.0f;
        const float layer2 = 180.0f;

        float xx = float(x + offsetx);
        float yy = float(y + offsety);
        return (((stb_perlin_noise3(xx / layer1, yy / layer1, 0, 0, 0, 0) + 1.0f) * 0.5f) + ((stb_perlin_noise3(xx / layer2, yy / layer2, 0, 0, 0, 0) + 1.0f) * 0.5f)) * 0.5f;
    }

    Terrain::Terrain(int width, int height, int lowside, int lowscale, float xRand, float yRand, float zRand, float texRandX, float texRandZ, int tileOriginX, int tileOriginZ)
    {
        LUMOS_PROFILE_FUNCTION();
        uint32_t numVertices = width * height;
        uint32_t numIndices  = (width - 1) * (height - 1) * 6;
        Vec3* vertices       = new Vec3[numVertices];
        Vec2* texCoords      = new Vec2[numVertices];
        uint32_t* indices    = new uint32_t[numIndices];
        m_BoundingBox        = {};

        m_GridWidth   = width;
        m_GridHeight  = height;
        m_XScale      = xRand;
        m_YScale      = yRand;
        m_ZScale      = zRand;
        m_TexRandX    = texRandX;
        m_TexRandZ    = texRandZ;
        m_TileOriginX = tileOriginX;
        m_TileOriginZ = tileOriginZ;
        m_HeightData.Resize(numVertices);

        for(int x = 0; x < width; ++x)
        {
            for(int z = 0; z < height; ++z)
            {
                int offset = (x * width) + z;

                float dataVal = Noise(x + tileOriginX, z + tileOriginZ);

                float rawHeight      = dataVal * dataVal * dataVal;
                m_HeightData[offset] = rawHeight;

                vertices[offset] = Vec3(
                    static_cast<float>(x) * xRand,
                    rawHeight * yRand,
                    static_cast<float>(z) * zRand);

                texCoords[offset] = Vec2(x * texRandX, z * texRandZ);
            }
        }

        int indicesCount = 0;

        for(int x = 0; x < width - 1; ++x)
        {
            for(int z = 0; z < height - 1; ++z)
            {
                if((uint32_t)indicesCount < numIndices - 6)
                {
                    int a = (x * (width)) + z;
                    int b = ((x + 1) * (width)) + z;
                    int c = ((x + 1) * (width)) + (z + 1);
                    int d = (x * (width)) + (z + 1);

                    indices[indicesCount++] = c;
                    indices[indicesCount++] = b;
                    indices[indicesCount++] = a;

                    indices[indicesCount++] = a;
                    indices[indicesCount++] = d;
                    indices[indicesCount++] = c;
                }
            }
        }

        Vec3* normals  = GenerateNormals(numVertices, vertices, indices, indicesCount);
        Vec3* tangents = GenerateTangents(numVertices, vertices, indices, indicesCount, texCoords);

        Graphics::Vertex* verts = new Graphics::Vertex[numVertices];

        for(uint32_t i = 0; i < numVertices; i++)
        {
            verts[i].Position  = vertices[i];
            verts[i].Colours   = Vec4(0.0f);
            verts[i].Normal    = normals[i];
            verts[i].TexCoords = texCoords[i];
            verts[i].Tangent   = tangents[i];

            m_BoundingBox.Merge(verts[i].Position);
        }

        m_VertexBuffer = SharedPtr<Graphics::VertexBuffer>(Graphics::VertexBuffer::Create(Graphics::BufferUsage::STATIC));
        m_VertexBuffer->SetData(sizeof(Graphics::Vertex) * numVertices, (void*)verts);

        m_IndexBuffer = SharedPtr<Graphics::IndexBuffer>(Graphics::IndexBuffer::Create(indices, indicesCount)); // / sizeof(uint32_t));

        delete[] normals;
        delete[] tangents;
        delete[] verts;
        delete[] vertices;
        delete[] indices;
        delete[] texCoords;
    }

    void Terrain::Rebuild(const float* heights)
    {
        LUMOS_PROFILE_FUNCTION();
        if(m_GridWidth <= 1 || m_GridHeight <= 1)
            return;

        const int width      = m_GridWidth;
        const int height     = m_GridHeight;
        const uint32_t numVertices = (uint32_t)(width * height);
        const uint32_t numIndices  = (uint32_t)((width - 1) * (height - 1) * 6);

        if(heights)
        {
            m_HeightData.Resize(numVertices);
            for(uint32_t i = 0; i < numVertices; i++)
                m_HeightData[i] = heights[i];
        }

        Vec3* vertices    = new Vec3[numVertices];
        Vec2* texCoords   = new Vec2[numVertices];
        uint32_t* indices = new uint32_t[numIndices];
        m_BoundingBox     = {};

        for(int x = 0; x < width; ++x)
        {
            for(int z = 0; z < height; ++z)
            {
                int offset       = (x * width) + z;
                float rawHeight  = m_HeightData[offset];
                vertices[offset] = Vec3(
                    static_cast<float>(x) * m_XScale,
                    rawHeight * m_YScale,
                    static_cast<float>(z) * m_ZScale);
                texCoords[offset] = Vec2(x * m_TexRandX, z * m_TexRandZ);
            }
        }

        int indicesCount = 0;
        for(int x = 0; x < width - 1; ++x)
        {
            for(int z = 0; z < height - 1; ++z)
            {
                if((uint32_t)indicesCount < numIndices - 6)
                {
                    int a = (x * width) + z;
                    int b = ((x + 1) * width) + z;
                    int c = ((x + 1) * width) + (z + 1);
                    int d = (x * width) + (z + 1);
                    indices[indicesCount++] = c;
                    indices[indicesCount++] = b;
                    indices[indicesCount++] = a;
                    indices[indicesCount++] = a;
                    indices[indicesCount++] = d;
                    indices[indicesCount++] = c;
                }
            }
        }

        Vec3* normals  = GenerateNormals(numVertices, vertices, indices, indicesCount);
        Vec3* tangents = GenerateTangents(numVertices, vertices, indices, indicesCount, texCoords);

        Graphics::Vertex* verts = new Graphics::Vertex[numVertices];
        for(uint32_t i = 0; i < numVertices; i++)
        {
            verts[i].Position  = vertices[i];
            verts[i].Colours   = Vec4(0.0f);
            verts[i].Normal    = normals[i];
            verts[i].TexCoords = texCoords[i];
            verts[i].Tangent   = tangents[i];
            m_BoundingBox.Merge(verts[i].Position);
        }

        m_VertexBuffer = SharedPtr<Graphics::VertexBuffer>(Graphics::VertexBuffer::Create(Graphics::BufferUsage::STATIC));
        m_VertexBuffer->SetData(sizeof(Graphics::Vertex) * numVertices, (void*)verts);
        m_IndexBuffer = SharedPtr<Graphics::IndexBuffer>(Graphics::IndexBuffer::Create(indices, indicesCount));

        delete[] normals;
        delete[] tangents;
        delete[] verts;
        delete[] vertices;
        delete[] indices;
        delete[] texCoords;
    }
}
