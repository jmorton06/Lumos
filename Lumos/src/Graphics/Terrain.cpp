#include "LM.h"
#include "Terrain.h"
#include "App/SceneManager.h"
#include "Maths/BoundingSphere.h"
#include "external/simplex/simplexnoise.h"

namespace Lumos
{
	Terrain::Terrain()
	{
		int xCoord = 0;
		int zCoord = 0;
		uint numVertices = RAW_WIDTH_RANDOM  * RAW_HEIGHT_RANDOM;
		uint numIndices = (RAW_WIDTH_RANDOM - 1) * (RAW_HEIGHT_RANDOM - 1) * 6;
		maths::Vector3* vertices = new maths::Vector3[numVertices];
		maths::Vector2* texCoords = new maths::Vector2[numVertices];
		uint* indices = new uint[numIndices];
        m_BoundingSphere = std::make_shared<maths::BoundingSphere>();

		float** lowMap = new float*[RAW_LOWSIDE_RANDOM + 1];

		for (int x = 0; x < RAW_LOWSIDE_RANDOM + 1; ++x)
		{
			lowMap[x] = new float[RAW_LOWSIDE_RANDOM + 1];
		}

		float** lowMapExpand = new float*[RAW_WIDTH_RANDOM];

		for (int x = 0; x < RAW_WIDTH_RANDOM; ++x)
		{
			lowMapExpand[x] = new float[RAW_HEIGHT_RANDOM];
		}


		for (int x = 0; x < RAW_LOWSIDE_RANDOM + 1; ++x) 
		{
			for (int z = 0; z < RAW_LOWSIDE_RANDOM + 1; ++z) 
			{
				lowMap[x][z] = (octave_noise_2d(1.0f, 0.1f, 0.01f, static_cast<float>(x) + static_cast<float>(xCoord * RAW_LOWSIDE_RANDOM), static_cast<float>(z) + static_cast<float>(zCoord * RAW_LOWSIDE_RANDOM)) / 2.0f) + 0.5f;
			}
		}

		for (int x = 0; x < RAW_WIDTH_RANDOM; ++x) 
		{
			for (int z = 0; z < RAW_HEIGHT_RANDOM; ++z) 
			{
				lowMapExpand[x][z] = 0.1f;
			}
		}

		for (int x = 0; x < (RAW_LOWSIDE_RANDOM); ++x) 
		{
			for (int lx = 0; lx < (RAW_LOWSCALE_RANDOM); ++lx) 
			{
				int currXCoord = (x * RAW_LOWSCALE_RANDOM) + lx;

				for (int z = 0; z < (RAW_LOWSIDE_RANDOM); ++z)
				{
					for (int lz = 0; lz < RAW_LOWSCALE_RANDOM; ++lz) 
					{
						int currZCoord = (z * RAW_LOWSCALE_RANDOM) + lz;

						float topL = lowMap[x + 1][z + 1];
						float topR = lowMap[x][z + 1];
						float botL = lowMap[x + 1][z];
						float botR = lowMap[x][z];

						float xScaleWeight = (static_cast<float>(lx) / (RAW_LOWSCALE_RANDOM));
						float xScaleTemp1 =
							(xScaleWeight * topL) +
							((1.0f - xScaleWeight) * topR);
						float xScaleTemp2 =
							(xScaleWeight * botL) +
							((1.0f - xScaleWeight) * botR);

						float yScaleWeight = (static_cast<float>(lz) / (RAW_LOWSCALE_RANDOM));
						float temp =
							yScaleWeight * xScaleTemp1 +
							(1.0f - yScaleWeight) * xScaleTemp2;

						lowMapExpand[currXCoord][currZCoord] = temp;
					}
				}
			}
		}

		for (int x = 0; x < RAW_WIDTH_RANDOM; ++x) 
		{
			for (int z = 0; z < RAW_HEIGHT_RANDOM; ++z) 
			{
				int offset = (x * RAW_WIDTH_RANDOM) + z;

				float lowWeight = 0.4f;
				//float lowWeight = 0.0f;
				float normWeight = 1.0f - lowWeight;

				float dataVal = (
					(lowMapExpand[x][z] * lowWeight) +
					(((octave_noise_2d(5, 0.45f, 0.01f,
					static_cast<float>(x) + (static_cast<float>(xCoord) * RAW_WIDTH_RANDOM),
					static_cast<float>(z) + (static_cast<float>(zCoord) * RAW_WIDTH_RANDOM)) / 2.0f) + 0.5f) * normWeight)
					);

				vertices[offset] = maths::Vector3(
					(static_cast<float>(x) + (static_cast<float>(xCoord) * RAW_WIDTH_RANDOM)) * HEIGHTMAP_X_RANDOM,
					(dataVal * dataVal * dataVal) * HEIGHTMAP_Y_RANDOM,
					(static_cast<float>(z) + static_cast<float>(zCoord * RAW_WIDTH_RANDOM)) * HEIGHTMAP_Z_RANDOM
					);

				texCoords[offset] = maths::Vector2(x * HEIGHTMAP_TEX_X_RANDOM, z * HEIGHTMAP_TEX_Z_RANDOM);
			}
		}

		numIndices = 0;

		for (int x = 0; x < RAW_WIDTH_RANDOM - 1; ++x) 
		{
			for (int z = 0; z < RAW_HEIGHT_RANDOM - 1; ++z) 
			{
				int a = (x      * (RAW_WIDTH_RANDOM)) + z;
				int b = ((x + 1) * (RAW_WIDTH_RANDOM)) + z;
				int c = ((x + 1) * (RAW_WIDTH_RANDOM)) + (z + 1);
				int d = (x      * (RAW_WIDTH_RANDOM)) + (z + 1);

				indices[numIndices++] = c;
				indices[numIndices++] = b;
				indices[numIndices++] = a;

				indices[numIndices++] = a;
				indices[numIndices++] = d;
				indices[numIndices++] = c;
			}
		}

		maths::Vector3* normals = GenerateNormals(numVertices, vertices, indices, numIndices);
		maths::Vector3* tangents = GenerateTangents(numVertices, vertices, indices, numIndices, texCoords);

		Vertex* verts = new Vertex[numVertices];

		for (uint i = 0; i < numVertices; i++)
		{
			verts[i].Position = vertices[i];
			verts[i].Colours = maths::Vector4(0.0f);
			verts[i].Normal = normals[i];
			verts[i].TexCoords = texCoords[i];
			verts[i].Tangent = tangents[i];
            
            m_BoundingSphere->ExpandToFit(verts[i].Position);
		}

		m_VertexArray = std::shared_ptr<VertexArray>(VertexArray::Create());

		VertexBuffer* buffer = VertexBuffer::Create(BufferUsage::STATIC);
		buffer->SetData(sizeof(Vertex) * numVertices, (void*)verts);

		graphics::BufferLayout layout;
		layout.Push<maths::Vector3>("position");
		layout.Push<maths::Vector4>("colour");
		layout.Push<maths::Vector2>("texCoord");
		layout.Push<maths::Vector3>("normal");
		layout.Push<maths::Vector3>("tangent");
		buffer->SetLayout(layout);

		m_VertexArray->PushBuffer(buffer);

		m_IndexBuffer = std::shared_ptr<IndexBuffer>(IndexBuffer::Create(indices, numIndices));// / sizeof(uint));
        
        delete[] normals;
        delete[] tangents;
        delete[] verts;
        delete[] vertices;
        delete[] indices;
        delete[] texCoords;

		for (int x = 0; x < RAW_LOWSIDE_RANDOM + 1; ++x)
		{
			delete[] lowMap[x];
		}

		for (int x = 0; x < RAW_WIDTH_RANDOM; ++x)
		{
			delete[] lowMapExpand[x];
		}

		delete[] lowMap;
		delete[] lowMapExpand;

		Init();
	}
}
