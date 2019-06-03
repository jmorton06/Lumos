#include "LM.h"
#include "MeshFactory.h"
#include "Maths/BoundingSphere.h"
#include "Mesh.h"
#include "Material.h"
#include "API/VertexArray.h"
#include "Maths/Maths.h"
#include "Maths/MathsUtilities.h"

namespace Lumos
{
	namespace Graphics
	{
		Mesh* CreateQuad(float x, float y, float width, float height, std::shared_ptr<Material> material)
		{
			struct QuadVertex
			{
				Maths::Vector3 position;
				Maths::Vector2 uv;
			};

			QuadVertex* data = new QuadVertex[4];

			data[0].position = Maths::Vector3(x, y, 0.0f);
			data[0].uv = Maths::Vector2(0.0f, 1.0f);

			data[1].position = Maths::Vector3(x + width, y, 0.0f);
			data[1].uv = Maths::Vector2(0, 0);

			data[2].position = Maths::Vector3(x + width, y + height, 0.0f);
			data[2].uv = Maths::Vector2(1, 0);

			data[3].position = Maths::Vector3(x, y + height, 0.0f);
			data[3].uv = Maths::Vector2(1, 1);

			std::shared_ptr<VertexArray> va;
			va.reset(VertexArray::Create());

			VertexBuffer* buffer = VertexBuffer::Create(BufferUsage::STATIC);
			buffer->SetData(sizeof(QuadVertex) * 4, data);

			delete[] data;

			Graphics::BufferLayout layout;
			layout.Push<Maths::Vector3>("position");
			layout.Push<Maths::Vector2>("texCoord");
			buffer->SetLayout(layout);

			va->PushBuffer(buffer);
			uint indices[6] = { 0, 1, 2, 2, 3, 0, };
			std::shared_ptr<IndexBuffer> ib;
			ib.reset(IndexBuffer::Create(indices, 6));

			std::shared_ptr<Maths::BoundingSphere> boundingBox = std::make_shared<Maths::BoundingSphere>();
			for (int i = 0; i < 4; i++)
			{
				boundingBox->ExpandToFit(data[i].position);
			}
			return new Mesh(va, ib, material, boundingBox);
		}

		Mesh* CreateQuad(const Maths::Vector2& position, const Maths::Vector2& size, std::shared_ptr<Material>  material)
		{
			return CreateQuad(position.GetX(), position.GetY(), size.GetX(), size.GetY(), std::move(material));
		}

		Mesh* CreateQuad()
		{
			Vertex* data = new Vertex[4];

			data[0].Position = Maths::Vector3(-1.0f, -1.0f, 0.0f);
			data[0].TexCoords = Maths::Vector2(0.0f, 0.0f);
			data[0].Colours = Lumos::Maths::Vector4(0.0f);

			data[1].Position = Maths::Vector3(1.0f, -1.0f, 0.0f);
			data[1].Colours = Lumos::Maths::Vector4(0.0f);
			data[1].TexCoords = Maths::Vector2(1.0f, 0.0f);

			data[2].Position = Maths::Vector3(1.0f, 1.0f, 0.0f);
			data[2].Colours = Lumos::Maths::Vector4(0.0f);
			data[2].TexCoords = Maths::Vector2(1.0f, 1.0f);

			data[3].Position = Maths::Vector3(-1.0f, 1.0f, 0.0f);
			data[3].Colours = Lumos::Maths::Vector4(0.0f);
			data[3].TexCoords = Maths::Vector2(0.0f, 1.0f);

			std::shared_ptr<VertexArray> va;
			va.reset(VertexArray::Create());

			VertexBuffer* buffer = VertexBuffer::Create(BufferUsage::STATIC);
			buffer->SetData(sizeof(Vertex) * 4, data);

			delete[] data;

			Graphics::BufferLayout layout;
			layout.Push<Maths::Vector3>("position");
			layout.Push<Maths::Vector4>("colour");
			layout.Push<Maths::Vector2>("texCoord");
			layout.Push<Maths::Vector3>("normal");
			layout.Push<Maths::Vector3>("tangent");
			buffer->SetLayout(layout);

			va->PushBuffer(buffer);
			uint32 indices[6] = { 0, 1, 2, 2, 3, 0, };
			std::shared_ptr<IndexBuffer> ib;
			ib.reset(IndexBuffer::Create(indices, 6));

			std::shared_ptr<Material> empty;
			std::shared_ptr<Maths::BoundingSphere> boundingSphere = std::make_shared<Maths::BoundingSphere>();
			for (int i = 0; i < 4; i++)
			{
				boundingSphere->ExpandToFit(data[i].Position);
			}
			return new Mesh(va, ib, empty, boundingSphere);
		}

		Mesh* CreateCube(float size, std::shared_ptr<Material> material)
		{

			//    v6----- v5
			//   /|      /|
			//  v1------v0|
			//  | |     | |
			//  | |v7---|-|v4
			//  |/      |/
			//  v2------v3

			Vertex* data = new Vertex[24];

			data[0].Position = Maths::Vector3(1.0f, 1.0f, 1.0f);
			data[0].Colours = Maths::Vector4(0.0f);
			data[0].TexCoords = Maths::Vector2(1.0f, 1.0f);
			data[0].Normal = Maths::Vector3(0.0f, 0.0f, 1.0f);

			data[1].Position = Maths::Vector3(-1.0f, 1.0f, 1.0f);
			data[1].Colours = Maths::Vector4(0.0f);
			data[1].TexCoords = Maths::Vector2(1.0f, 0.0f);
			data[1].Normal = Maths::Vector3(0.0f, 0.0f, 1.0f);

			data[2].Position = Maths::Vector3(-1.0f, -1.0f, 1.0f);
			data[2].Colours = Maths::Vector4(0.0f);
			data[2].TexCoords = Maths::Vector2(0.0f, 0.0f);
			data[2].Normal = Maths::Vector3(0.0f, 0.0f, 1.0f);

			data[3].Position = Maths::Vector3(1.0f, -1.0f, 1.0f);
			data[3].Colours = Maths::Vector4(0.0f);
			data[3].TexCoords = Maths::Vector2(1.0f, 0.0f);
			data[3].Normal = Maths::Vector3(0.0f, 0.0f, 1.0f);

			data[4].Position = Maths::Vector3(1.0f, 1.0f, 1.0f);
			data[4].Colours = Maths::Vector4(0.0f);
			data[4].TexCoords = Maths::Vector2(1.0f, 1.0f);
			data[4].Normal = Maths::Vector3(1.0f, 0.0f, 0.0f);

			data[5].Position = Maths::Vector3(1.0f, -1.0f, 1.0f);
			data[5].Colours = Maths::Vector4(0.0f);
			data[5].TexCoords = Maths::Vector2(1.0f, 0.0f);
			data[5].Normal = Maths::Vector3(1.0f, 0.0f, 0.0f);

			data[6].Position = Maths::Vector3(1.0f, -1.0f, -1.0f);
			data[6].Colours = Maths::Vector4(0.0f);
			data[6].TexCoords = Maths::Vector2(0.0f, 0.0f);
			data[6].Normal = Maths::Vector3(1.0f, 0.0f, 0.0f);

			data[7].Position = Maths::Vector3(1.0f, 1.0f, -1.0f);
			data[7].Colours = Maths::Vector4(0.0f);
			data[7].TexCoords = Maths::Vector2(0.0f, 1.0f);
			data[7].Normal = Maths::Vector3(1.0f, 0.0f, 0.0f);

			data[8].Position = Maths::Vector3(1.0f, 1.0f, 1.0f);
			data[8].Colours = Maths::Vector4(0.0f);
			data[8].Normal = Maths::Vector3(0.0f, 1.0f, 0.0f);

			data[9].Position = Maths::Vector3(1.0f, 1.0f, -1.0f);
			data[9].Colours = Maths::Vector4(0.0f);
			data[9].Normal = Maths::Vector3(0.0f, 1.0f, 0.0f);

			data[10].Position = Maths::Vector3(-1.0f, 1.0f, -1.0f);
			data[10].Colours = Maths::Vector4(0.0f);
			data[10].TexCoords = Maths::Vector2(0.0f, 1.0f);
			data[10].Normal = Maths::Vector3(0.0f, 1.0f, 0.0f);

			data[11].Position = Maths::Vector3(-1.0f, 1.0f, 1.0f);
			data[11].Colours = Maths::Vector4(0.0f);
			data[11].Normal = Maths::Vector3(0.0f, 1.0f, 0.0f);

			data[12].Position = Maths::Vector3(-1.0f, 1.0f, 1.0f);
			data[12].Colours = Maths::Vector4(0.0f);
			data[12].Normal = Maths::Vector3(-1.0f, 0.0f, 0.0f);

			data[13].Position = Maths::Vector3(-1.0f, 1.0f, -1.0f);
			data[13].Colours = Maths::Vector4(0.0f);
			data[13].Normal = Maths::Vector3(-1.0f, 0.0f, 0.0f);

			data[14].Position = Maths::Vector3(-1.0f, -1.0f, -1.0f);
			data[14].Colours = Maths::Vector4(0.0f);
			data[14].Normal = Maths::Vector3(-1.0f, 0.0f, 0.0f);

			data[15].Position = Maths::Vector3(-1.0f, -1.0f, 1.0f);
			data[15].Colours = Maths::Vector4(0.0f);
			data[15].Normal = Maths::Vector3(-1.0f, 0.0f, 0.0f);

			data[16].Position = Maths::Vector3(-1.0f, -1.0f, -1.0f);
			data[16].Colours = Maths::Vector4(0.0f);
			data[16].Normal = Maths::Vector3(0.0f, -1.0f, 0.0f);

			data[17].Position = Maths::Vector3(1.0f, -1.0f, -1.0f);
			data[17].Colours = Maths::Vector4(0.0f);
			data[17].Normal = Maths::Vector3(0.0f, -1.0f, 0.0f);

			data[18].Position = Maths::Vector3(1.0f, -1.0f, 1.0f);
			data[18].Colours = Maths::Vector4(0.0f);
			data[18].Normal = Maths::Vector3(0.0f, -1.0f, 0.0f);

			data[19].Position = Maths::Vector3(-1.0f, -1.0f, 1.0f);
			data[19].Colours = Maths::Vector4(0.0f);
			data[19].Normal = Maths::Vector3(0.0f, -1.0f, 0.0f);

			data[20].Position = Maths::Vector3(1.0f, -1.0f, -1.0f);
			data[20].Colours = Maths::Vector4(0.0f);
			data[20].Normal = Maths::Vector3(0.0f, 0.0f, -1.0f);

			data[21].Position = Maths::Vector3(-1.0f, -1.0f, -1.0f);
			data[21].Colours = Maths::Vector4(0.0f);
			data[21].Normal = Maths::Vector3(0.0f, 0.0f, -1.0f);

			data[22].Position = Maths::Vector3(-1.0f, 1.0f, -1.0f);
			data[22].Colours = Maths::Vector4(0.0f);
			data[22].Normal = Maths::Vector3(0.0f, 0.0f, -1.0f);

			data[23].Position = Maths::Vector3(1.0f, 1.0f, -1.0f);
			data[23].Colours = Maths::Vector4(0.0f);
			data[23].Normal = Maths::Vector3(0.0f, 0.0f, -1.0f);

			std::shared_ptr<VertexArray> va;
			va.reset(VertexArray::Create());

			for (int i = 0; i < 6; i++)
			{
				data[i * 4 + 0].TexCoords = Maths::Vector2(0.0f, 0.0f);
				data[i * 4 + 1].TexCoords = Maths::Vector2(1.0f, 0.0f);
				data[i * 4 + 2].TexCoords = Maths::Vector2(1.0f, 1.0f);
				data[i * 4 + 3].TexCoords = Maths::Vector2(0.0f, 1.0f);

			}

			VertexBuffer* buffer = VertexBuffer::Create(BufferUsage::STATIC);
			buffer->SetData(24 * sizeof(Vertex), data);

			delete[] data;

			Graphics::BufferLayout layout;
			layout.Push<Maths::Vector3>("position");
			layout.Push<Maths::Vector4>("colour");
			layout.Push<Maths::Vector2>("texCoord");
			layout.Push<Maths::Vector3>("normal");
			layout.Push<Maths::Vector3>("tangent");
			buffer->SetLayout(layout);

			va->Bind();
			va->PushBuffer(buffer);

			uint indices[36]
			{
				0,1,2,
				0,2,3,
				4,5,6,
				4,6,7,
				8,9,10,
				8,10,11,
				12,13,14,
				12,14,15,
				16,17,18,
				16,18,19,
				20,21,22,
				20,22,23
			};

			std::shared_ptr<IndexBuffer> ib;
			ib.reset(IndexBuffer::Create(indices, 36));
			std::shared_ptr<Maths::BoundingSphere> boundingSphere = std::make_shared<Maths::BoundingSphere>();
			for (int i = 0; i < 4; i++)
			{
				boundingSphere->ExpandToFit(data[i].Position);
			}
			return new Mesh(va, ib, material, boundingSphere);
		}

		Mesh* CreatePyramid(float size, std::shared_ptr<Material> material)
		{
			Vertex* data = new Vertex[18];

			data[0].Position = Maths::Vector3(1.0f, 1.0f, -1.0f);
			data[0].Colours = Maths::Vector4(0.0f);
			data[0].TexCoords = Maths::Vector2(0.24f, 0.20f);
			data[0].Normal = Maths::Vector3(0.0f, 0.8948f, 0.4464f);

			data[1].Position = Maths::Vector3(-1.0f, 1.0f, -1.0f);
			data[1].Colours = Maths::Vector4(0.0f);
			data[1].TexCoords = Maths::Vector2(0.24f, 0.81f);
			data[1].Normal = Maths::Vector3(0.0f, 0.8948f, 0.4464f);

			data[2].Position = Maths::Vector3(0.0f, 0.0f, 1.0f);
			data[2].Colours = Maths::Vector4(0.0f);
			data[2].TexCoords = Maths::Vector2(0.95f, 0.50f);
			data[2].Normal = Maths::Vector3(0.0f, 0.8948f, 0.4464f);

			data[3].Position = Maths::Vector3(-1.0f, 1.0f, -1.0f);
			data[3].Colours = Maths::Vector4(0.0f);
			data[3].TexCoords = Maths::Vector2(0.24f, 0.21f);
			data[3].Normal = Maths::Vector3(-0.8948f, 0.0f, 0.4464f);

			data[4].Position = Maths::Vector3(-1.0f, -1.0f, -1.0f);
			data[4].Colours = Maths::Vector4(0.0f);
			data[4].TexCoords = Maths::Vector2(0.24f, 0.81f);
			data[4].Normal = Maths::Vector3(-0.8948f, 0.0f, 0.4464f);

			data[5].Position = Maths::Vector3(0.0f, 0.0f, 1.0f);
			data[5].Colours = Maths::Vector4(0.0f);
			data[5].TexCoords = Maths::Vector2(0.95f, 0.50f);
			data[5].Normal = Maths::Vector3(-0.8948f, 0.0f, 0.4464f);

			data[6].Position = Maths::Vector3(1.0f, 1.0f, -1.0f);
			data[6].Colours = Maths::Vector4(0.0f);
			data[6].TexCoords = Maths::Vector2(0.24f, 0.81f);
			data[6].Normal = Maths::Vector3(0.8948f, 0.0f, 0.4475f);

			data[7].Position = Maths::Vector3(0.0f, 0.0f, 1.0f);
			data[7].Colours = Maths::Vector4(0.0f);
			data[7].TexCoords = Maths::Vector2(0.95f, 0.50f);
			data[7].Normal = Maths::Vector3(0.8948f, 0.0f, 0.4475f);

			data[8].Position = Maths::Vector3(1.0f, -1.0f, -1.0f);
			data[8].Colours = Maths::Vector4(0.0f);
			data[8].TexCoords = Maths::Vector2(0.24f, 0.21f);
			data[8].Normal = Maths::Vector3(0.8948f, 0.0f, 0.4475f);

			data[9].Position = Maths::Vector3(-1.0f, -1.0f, -1.0f);
			data[9].Colours = Maths::Vector4(0.0f);
			data[9].TexCoords = Maths::Vector2(0.24f, 0.21f);
			data[9].Normal = Maths::Vector3(0.0f, -0.8948f, 0.448f);

			data[10].Position = Maths::Vector3(1.0f, -1.0f, -1.0f);
			data[10].Colours = Maths::Vector4(0.0f);
			data[10].TexCoords = Maths::Vector2(0.24f, 0.81f);
			data[10].Normal = Maths::Vector3(0.0f, -0.8948f, 0.448f);

			data[11].Position = Maths::Vector3(0.0f, 0.0f, 1.0f);
			data[11].Colours = Maths::Vector4(0.0f);
			data[11].TexCoords = Maths::Vector2(0.95f, 0.50f);
			data[11].Normal = Maths::Vector3(0.0f, -0.8948f, 0.448f);

			data[12].Position = Maths::Vector3(-1.0f, 1.0f, -1.0f);
			data[12].Colours = Maths::Vector4(0.0f);
			data[12].TexCoords = Maths::Vector2(0.0f, 0.0f);
			data[12].Normal = Maths::Vector3(0.0f, 0.0f, -1.0f);

			data[13].Position = Maths::Vector3(1.0f, 1.0f, -1.0f);
			data[13].Colours = Maths::Vector4(0.0f);
			data[13].TexCoords = Maths::Vector2(0.0f, 1.0f);
			data[13].Normal = Maths::Vector3(0.0f, 0.0f, -1.0f);

			data[14].Position = Maths::Vector3(1.0f, -1.0f, -1.0f);
			data[14].Colours = Maths::Vector4(0.0f);
			data[14].TexCoords = Maths::Vector2(1.0f, 1.0f);
			data[14].Normal = Maths::Vector3(0.0f, 0.0f, -1.0f);

			data[15].Position = Maths::Vector3(-1.0f, -1.0f, -1.0f);
			data[15].Colours = Maths::Vector4(0.0f);
			data[15].TexCoords = Maths::Vector2(0.96f, 0.0f);
			data[15].Normal = Maths::Vector3(0.0f, 0.0f, -1.0f);

			data[16].Position = Maths::Vector3(0.0f, 0.0f, 0.0f);
			data[16].Colours = Maths::Vector4(0.0f);
			data[16].TexCoords = Maths::Vector2(0.0f, 0.0f);
			data[16].Normal = Maths::Vector3(0.0f, 0.0f, 0.0f);

			data[17].Position = Maths::Vector3(0.0f, 0.0f, 0.0f);
			data[17].Colours = Maths::Vector4(0.0f);
			data[17].TexCoords = Maths::Vector2(0.0f, 0.0f);
			data[17].Normal = Maths::Vector3(0.0f, 0.0f, 0.0f);

			std::shared_ptr<VertexArray> va;
			va.reset(VertexArray::Create());

			VertexBuffer* buffer = VertexBuffer::Create(BufferUsage::STATIC);
			buffer->SetData(18 * sizeof(Vertex), data);

			delete[] data;

			Graphics::BufferLayout layout;
			layout.Push<Maths::Vector3>("position");
			layout.Push<Maths::Vector4>("colour");
			layout.Push<Maths::Vector2>("texCoord");
			layout.Push<Maths::Vector3>("normal");
			layout.Push<Maths::Vector3>("tangent");
			buffer->SetLayout(layout);

			va->Bind();
			va->PushBuffer(buffer);

			uint indices[18]
			{
				0,1,2,
				3,4,5,
				6,7,8,
				9,10,11,
				12,13,14,
				15,12,14
			};

			std::shared_ptr<IndexBuffer> ib;
			ib.reset(IndexBuffer::Create(indices, 18));
			std::shared_ptr<Maths::BoundingSphere> boundingSphere = std::make_shared<Maths::BoundingSphere>();
			for (int i = 0; i < 4; i++)
			{
				boundingSphere->ExpandToFit(data[i].Position);
			}
			return new Mesh(va, ib, material, boundingSphere);
		}

		Mesh* CreateSphere(uint xSegments, uint ySegments, std::shared_ptr<Material> material)
		{
			auto data = std::vector<Vertex>();

			std::shared_ptr<VertexArray> va;
			va.reset(VertexArray::Create());

			float sectorCount = static_cast<float>(xSegments);
			float stackCount = static_cast<float>(ySegments);
			float sectorStep = 2 * Maths::PI / sectorCount;
			float stackStep = Maths::PI / stackCount;
			float radius = 1.0f;

			for (int i = 0; i <= stackCount; ++i)
			{
                float stackAngle = Maths::PI / 2 - i * stackStep;        // starting from pi/2 to -pi/2
				float xy = radius * cos(stackAngle);             // r * cos(u)
				float z = radius * sin(stackAngle);              // r * sin(u)

				// add (sectorCount+1) vertices per stack
				// the first and last vertices have same position and normal, but different tex coords
				for (int j = 0; j <= sectorCount; ++j)
				{
                    float sectorAngle = j * sectorStep;           // starting from 0 to 2pi

					// vertex position (x, y, z)
					float x = xy * cosf(sectorAngle);             // r * cos(u) * cos(v)
					float y = xy * sinf(sectorAngle);             // r * cos(u) * sin(v)

					// vertex tex coord (s, t) range between [0, 1]
					float s = static_cast<float>(j / sectorCount);
					float t = static_cast<float>(i / stackCount);

					Vertex vertex;
					vertex.Position = Maths::Vector3(x, y, z);
					vertex.TexCoords = Maths::Vector2(s, t);
					vertex.Normal = Maths::Vector3(x, y, z).Normal();

					data.emplace_back(vertex);
				}
			}

			VertexBuffer* buffer = VertexBuffer::Create(BufferUsage::STATIC);
			buffer->SetData(int(data.size()) * sizeof(Vertex), data.data());

			Graphics::BufferLayout layout;
			layout.Push<Maths::Vector3>("position");
			layout.Push<Maths::Vector4>("colour");
			layout.Push<Maths::Vector2>("texCoord");
			layout.Push<Maths::Vector3>("normal");
			layout.Push<Maths::Vector3>("tangent");
			buffer->SetLayout(layout);

			va->Bind();
			va->PushBuffer(buffer);

			std::vector<uint> indices;
			uint k1, k2;
			for (uint i = 0; i < stackCount; ++i)
			{
				k1 = i * (static_cast<uint>(sectorCount) + 1U);     // beginning of current stack
				k2 = k1 + static_cast<uint>(sectorCount) + 1U;      // beginning of next stack

				for (uint j = 0; j < sectorCount; ++j, ++k1, ++k2)
				{
					// 2 triangles per sector excluding first and last stacks
					// k1 => k2 => k1+1
					if (i != 0)
					{
						indices.push_back(k1);
						indices.push_back(k2);
						indices.push_back(k1 + 1);
					}

					// k1+1 => k2 => k2+1
					if (i != (stackCount - 1))
					{
						indices.push_back(k1 + 1);
						indices.push_back(k2);
						indices.push_back(k2 + 1);
					}
				}
			}

			std::shared_ptr<IndexBuffer> ib;
			ib.reset(IndexBuffer::Create(indices.data(), static_cast<uint>(indices.size())));
			std::shared_ptr<Maths::BoundingSphere> boundingSphere = std::make_shared<Maths::BoundingSphere>();

			return new Mesh(va, ib, material, boundingSphere);
		}

		Mesh* CreateIcoSphere(uint radius, uint subdivision, std::shared_ptr<Material> material)
		{
			return nullptr;
		}

		Mesh* CreatePlane(float width, float height, const Maths::Vector3& normal, std::shared_ptr<Material> material)
		{
			Maths::Vector3 vec = normal * 90.0f;
			Maths::Matrix4 rotation = Maths::Matrix4::Rotation(vec.z, Maths::Vector3(1.0f, 0.0f, 0.0f)) * Maths::Matrix4::Rotation(vec.y, Maths::Vector3(0.0f, 1.0f, 0.0f)) * Maths::Matrix4::Rotation(vec.x, Maths::Vector3(0.0f, 0.0f, 1.0f));

			Vertex data[4];
			memset(data, 0, 4 * sizeof(Vertex));

			data[0].Position = rotation * Maths::Vector3(-width / 2.0f, 0.0f, -height / 2.0f);
			data[0].Normal = normal;
			data[0].TexCoords = Maths::Vector2(0.0f, 0.0f);
			data[0].Tangent = Maths::Matrix4::Rotation(90.0f, Maths::Vector3(0.0f, 0.0f, 1.0f)) * normal;

			data[1].Position = rotation * Maths::Vector3(-width / 2.0f, 0.0f, height / 2.0f);
			data[1].Normal = normal;
			data[1].TexCoords = Maths::Vector2(0.0f, 1.0f);
			data[1].Tangent = Maths::Matrix4::Rotation(90.0f, Maths::Vector3(0, 0, 1)) * normal;

			data[2].Position = rotation * Maths::Vector3(width / 2.0f, 0.0f, height / 2.0f);
			data[2].Normal = normal;
			data[2].TexCoords = Maths::Vector2(1.0f, 1.0f);
			data[2].Tangent = Maths::Matrix4::Rotation(90.0f, Maths::Vector3(0.0f, 0.0f, 1.0f)) * normal;

			data[3].Position = rotation * Maths::Vector3(width / 2.0f, 0.0f, -height / 2.0f);
			data[3].Normal = normal;
			data[3].TexCoords = Maths::Vector2(1.0f, 0.0f);
			data[3].Tangent = Maths::Matrix4::Rotation(90.0f, Maths::Vector3(0.0f, 0.0f, 1.0f)) * normal;

			VertexBuffer* buffer = VertexBuffer::Create(BufferUsage::STATIC);
			buffer->SetData(8 * sizeof(Vertex), data);

			//delete[] data;

			Graphics::BufferLayout layout;
			layout.Push<Maths::Vector3>("postion");
			layout.Push<Maths::Vector4>("colours");
			layout.Push<Maths::Vector2>("texCoord");
			layout.Push<Maths::Vector3>("normal");
			layout.Push<Maths::Vector3>("tangent");
			buffer->SetLayout(layout);

			std::shared_ptr<VertexArray> va;
			va.reset(VertexArray::Create());
			va->PushBuffer(buffer);

			uint indices[6]
			{
					0, 1, 2,
					2, 3, 0
			};

			std::shared_ptr<IndexBuffer> ib;
			ib.reset(IndexBuffer::Create(indices, 6));
			std::shared_ptr<Maths::BoundingSphere> boundingBox = std::make_shared<Maths::BoundingSphere>();
			for (int i = 0; i < 4; i++)
			{
				boundingBox->ExpandToFit(data[i].Position);
			}
			return new Mesh(va, ib, material, boundingBox);
		}
	}
}
