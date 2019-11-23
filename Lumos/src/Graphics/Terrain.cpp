#include "lmpch.h"
#include "Terrain.h"
#include "App/SceneManager.h"
#include "Maths/BoundingBox.h"
#include <simplex/simplexnoise.h>

namespace Lumos
{
	Terrain::Terrain(int width, int height, int lowside, int lowscale, float xRand, float yRand, float zRand, float texRandX, float texRandZ)
	{
		int xCoord = 0;
		int zCoord = 0;
		u32 numVertices = width  * height;
		u32 numIndices = (width - 1) * (height - 1) * 6;
		Maths::Vector3* vertices = lmnew Maths::Vector3[numVertices];
		Maths::Vector2* texCoords = lmnew Maths::Vector2[numVertices];
		u32* indices = lmnew u32[numIndices];
        m_BoundingBox = CreateRef<Maths::BoundingBox>();

		float** lowMap = lmnew float*[lowside + 1];

		for (int x = 0; x < lowside + 1; ++x)
		{
			lowMap[x] = lmnew float[lowside + 1];
		}

		float** lowMapExpand = lmnew float*[width];

		for (int x = 0; x < width; ++x)
		{
			lowMapExpand[x] = lmnew float[height];
		}


		for (int x = 0; x < lowside + 1; ++x)
		{
			for (int z = 0; z < lowside + 1; ++z)
			{
				lowMap[x][z] = (octave_noise_2d(1.0f, 0.1f, 0.01f, static_cast<float>(x) + static_cast<float>(xCoord * lowside), static_cast<float>(z) + static_cast<float>(zCoord * lowside)) / 2.0f) + 0.5f;
			}
		}

		for (int x = 0; x < width; ++x)
		{
			for (int z = 0; z < height; ++z)
			{
				lowMapExpand[x][z] = 0.1f;
			}
		}

		for (int x = 0; x < (lowside); ++x)
		{
			for (int lx = 0; lx < (lowscale); ++lx)
			{
				int currXCoord = (x * lowscale) + lx;

				for (int z = 0; z < (lowside); ++z)
				{
					for (int lz = 0; lz < lowscale; ++lz)
					{
						int currZCoord = (z * lowscale) + lz;

						float topL = lowMap[x + 1][z + 1];
						float topR = lowMap[x][z + 1];
						float botL = lowMap[x + 1][z];
						float botR = lowMap[x][z];

						float xScaleWeight = (static_cast<float>(lx) / (lowscale));
						float xScaleTemp1 =
							(xScaleWeight * topL) +
							((1.0f - xScaleWeight) * topR);
						float xScaleTemp2 =
							(xScaleWeight * botL) +
							((1.0f - xScaleWeight) * botR);

						float yScaleWeight = (static_cast<float>(lz) / (lowscale));
						float temp =
							yScaleWeight * xScaleTemp1 +
							(1.0f - yScaleWeight) * xScaleTemp2;

						lowMapExpand[currXCoord][currZCoord] = temp;
					}
				}
			}
		}

		for (int x = 0; x < width; ++x)
		{
			for (int z = 0; z < height; ++z)
			{
				int offset = (x * width) + z;

				float lowWeight = 0.4f;
				//float lowWeight = 0.0f;
				float normWeight = 1.0f - lowWeight;

				float dataVal = (
					(lowMapExpand[x][z] * lowWeight) +
					(((octave_noise_2d(5, 0.45f, 0.01f,
					static_cast<float>(x) + (static_cast<float>(xCoord) * width),
					static_cast<float>(z) + (static_cast<float>(zCoord) * width)) / 2.0f) + 0.5f) * normWeight)
					);

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

		Graphics::Vertex* verts = lmnew Graphics::Vertex[numVertices];

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
        
        lmdel[] normals;
        lmdel[] tangents;
        lmdel[] verts;
        lmdel[] vertices;
        lmdel[] indices;
        lmdel[] texCoords;

		for (int x = 0; x < lowside + 1; ++x)
		{
			lmdel[] lowMap[x];
		}

		for (int x = 0; x < width; ++x)
		{
			lmdel[] lowMapExpand[x];
		}

		lmdel[] lowMap;
		lmdel[] lowMapExpand;
	}
}
