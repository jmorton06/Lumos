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
		return
            (
			 ((stb_perlin_noise3(xx / layer1, yy / layer1, 0, 0, 0, 0) + 1.0f) / 2.0f) +
			 ((stb_perlin_noise3(xx / layer2, yy / layer2, 0, 0, 0, 0) + 1.0f) / 2.0f)
			 ) / 2.0f;
	}
	
	Terrain::Terrain(int width, int height, int lowside, int lowscale, float xRand, float yRand, float zRand, float texRandX, float texRandZ)
	{
		int xCoord = 0;
		int zCoord = 0;
		u32 numVertices = width  * height;
		u32 numIndices = (width - 1) * (height - 1) * 6;
		Maths::Vector3* vertices = new Maths::Vector3[numVertices];
		Maths::Vector2* texCoords = new Maths::Vector2[numVertices];
		u32* indices = new u32[numIndices];
        m_BoundingBox = CreateRef<Maths::BoundingBox>();

		for (int x = 0; x < width; ++x)
		{
			for (int z = 0; z < height; ++z)
			{
				int offset = (x * width) + z;

				float dataVal = Noise(static_cast<float>(x) + (static_cast<float>(xCoord) * width),
									  static_cast<float>(z) + (static_cast<float>(zCoord) * width));

				vertices[offset] = Maths::Vector3(
					(static_cast<float>(x) + (static_cast<float>(xCoord) * width)) * xRand,
					(dataVal * dataVal * dataVal) * yRand,
					(static_cast<float>(z) + static_cast<float>(zCoord * width)) * zRand
					);

				texCoords[offset] = Maths::Vector2(x * texRandX, z * texRandZ);
			}
		}

		numIndices = 0;

		for (int x = 0; x < width - 1; ++x)
		{
			for (int z = 0; z < height - 1; ++z)
			{
				int a = (x      * (width)) + z;
				int b = ((x + 1) * (width)) + z;
				int c = ((x + 1) * (width)) + (z + 1);
				int d = (x      * (width)) + (z + 1);

				indices[numIndices++] = c;
				indices[numIndices++] = b;
				indices[numIndices++] = a;

				indices[numIndices++] = a;
				indices[numIndices++] = d;
				indices[numIndices++] = c;
			}
		}

		Maths::Vector3* normals = GenerateNormals(numVertices, vertices, indices, numIndices);
		Maths::Vector3* tangents = GenerateTangents(numVertices, vertices, indices, numIndices, texCoords);

		Graphics::Vertex* verts = new Graphics::Vertex[numVertices];

		for (u32 i = 0; i < numVertices; i++)
		{
			verts[i].Position = vertices[i];
			verts[i].Colours = Maths::Vector4(0.0f);
			verts[i].Normal = normals[i];
			verts[i].TexCoords = texCoords[i];
			verts[i].Tangent = tangents[i];
            
            m_BoundingBox->Merge(verts[i].Position);
		}

		m_VertexArray = Ref<Graphics::VertexArray>(Graphics::VertexArray::Create());

		Graphics::VertexBuffer* buffer = Graphics::VertexBuffer::Create(Graphics::BufferUsage::STATIC);
		buffer->SetData(sizeof(Graphics::Vertex) * numVertices, (void*)verts);

		Graphics::BufferLayout layout;
		layout.Push<Maths::Vector3>("position");
		layout.Push<Maths::Vector4>("colour");
		layout.Push<Maths::Vector2>("texCoord");
		layout.Push<Maths::Vector3>("normal");
		layout.Push<Maths::Vector3>("tangent");
		buffer->SetLayout(layout);

		m_VertexArray->PushBuffer(buffer);

		m_IndexBuffer = Ref<Graphics::IndexBuffer>(Graphics::IndexBuffer::Create(indices, numIndices));// / sizeof(u32));
        
        delete[] normals;
        delete[] tangents;
        delete[] verts;
        delete[] vertices;
        delete[] indices;
        delete[] texCoords;
	}
}
