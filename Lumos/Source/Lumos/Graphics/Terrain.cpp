#include "Precompiled.h"
#include "Terrain.h"
#include "Maths/BoundingBox.h"
#include <stb/stb_perlin.h>

namespace Lumos
{
    float Noise(int x, int y)
    {
        const int offsetx = 100;
        const int offsety = -50;
        const float layer1 = 25.0f;
        const float layer2 = 180.0f;

        float xx = float(x + offsetx);
        float yy = float(y + offsety);
        return (((stb_perlin_noise3(xx / layer1, yy / layer1, 0, 0, 0, 0) + 1.0f) / 2.0f) + ((stb_perlin_noise3(xx / layer2, yy / layer2, 0, 0, 0, 0) + 1.0f) / 2.0f)) / 2.0f;
    }

    Terrain::Terrain(int width, int height, int lowside, int lowscale, float xRand, float yRand, float zRand, float texRandX, float texRandZ)
    {
        LUMOS_PROFILE_FUNCTION();
        int xCoord = 0;
        int zCoord = 0;
        uint32_t numVertices = width * height;
        uint32_t numIndices = (width - 1) * (height - 1) * 6;
        Maths::Vector3* vertices = new Maths::Vector3[numVertices];
        Maths::Vector2* texCoords = new Maths::Vector2[numVertices];
        uint32_t* indices = new uint32_t[numIndices];
        m_BoundingBox = CreateSharedRef<Maths::BoundingBox>();

        for(int x = 0; x < width; ++x)
        {
            for(int z = 0; z < height; ++z)
            {
                int offset = (x * width) + z;

                float dataVal = Noise(x + (xCoord * width),
                    z + (zCoord * width));

                vertices[offset] = Maths::Vector3(
                    (static_cast<float>(x) + (static_cast<float>(xCoord) * float(width))) * xRand,
                    (dataVal * dataVal * dataVal) * yRand,
                    (static_cast<float>(z) + (static_cast<float>(zCoord) * float(width))) * zRand);

                texCoords[offset] = Maths::Vector2(x * texRandX, z * texRandZ);
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

        Maths::Vector3* normals = GenerateNormals(numVertices, vertices, indices, indicesCount);
        Maths::Vector3* tangents = GenerateTangents(numVertices, vertices, indices, indicesCount, texCoords);

        Graphics::Vertex* verts = new Graphics::Vertex[numVertices];

        for(uint32_t i = 0; i < numVertices; i++)
        {
            verts[i].Position = vertices[i];
            verts[i].Colours = Maths::Vector4(0.0f);
            verts[i].Normal = normals[i];
            verts[i].TexCoords = texCoords[i];
            verts[i].Tangent = tangents[i];

            m_BoundingBox->Merge(verts[i].Position);
        }

        m_VertexBuffer = SharedRef<Graphics::VertexBuffer>(Graphics::VertexBuffer::Create(Graphics::BufferUsage::STATIC));
        m_VertexBuffer->SetData(sizeof(Graphics::Vertex) * numVertices, (void*)verts);

        m_IndexBuffer = SharedRef<Graphics::IndexBuffer>(Graphics::IndexBuffer::Create(indices, indicesCount)); // / sizeof(uint32_t));

        delete[] normals;
        delete[] tangents;
        delete[] verts;
        delete[] vertices;
        delete[] indices;
        delete[] texCoords;
    }
}
