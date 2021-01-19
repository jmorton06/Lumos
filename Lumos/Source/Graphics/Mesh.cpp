#include "Precompiled.h"
#include "Mesh.h"
#include "API/Renderer.h"

namespace Lumos
{
	namespace Graphics
	{
		Mesh::Mesh() : m_VertexBuffer(nullptr), m_IndexBuffer(nullptr), m_BoundingBox(nullptr)
		{
		}

		Mesh::Mesh(const Mesh& mesh)
            : m_VertexBuffer(mesh.m_VertexBuffer), m_IndexBuffer(mesh.m_IndexBuffer), m_BoundingBox(mesh.m_BoundingBox), m_Name(mesh.m_Name), m_Material(mesh.m_Material)
		{
		}

		Mesh::Mesh(Ref<VertexBuffer>& vertexBuffer, Ref<IndexBuffer>& indexBuffer, const Ref<Maths::BoundingBox>& boundingBox)
			: m_VertexBuffer(vertexBuffer), m_IndexBuffer(indexBuffer), m_BoundingBox(boundingBox), m_Material(nullptr)
        {
		}

		Mesh::~Mesh()
		{
		}

		Maths::Vector3 Mesh::GenerateTangent(const Maths::Vector3 &a, const Maths::Vector3 &b, const Maths::Vector3 &c, const Maths::Vector2 &ta, const Maths::Vector2 &tb, const Maths::Vector2 &tc)
		{
			const Maths::Vector2 coord1 = tb - ta;
			const Maths::Vector2 coord2 = tc - ta;

			const Maths::Vector3 vertex1 = b - a;
			const Maths::Vector3 vertex2 = c - a;

			const Maths::Vector3 axis = Maths::Vector3(vertex1*coord2.y - vertex2 * coord1.y);

			const float factor = 1.0f / (coord1.x * coord2.y - coord2.x * coord1.y);

			return axis * factor;
		}

		Maths::Vector3* Mesh::GenerateNormals(u32 numVertices, Maths::Vector3* vertices, u32* indices, u32 numIndices)
		{
			Maths::Vector3* normals = new Maths::Vector3[numVertices];

			for (u32 i = 0; i < numVertices; ++i)
			{
				normals[i] = Maths::Vector3();
			}

			if (indices)
			{
				for (u32 i = 0; i < numIndices; i += 3)
				{
					const int a = indices[i];
					const int b = indices[i + 1];
					const int c = indices[i + 2];

					const Maths::Vector3 _normal = Maths::Vector3::Cross((vertices[b] - vertices[a]), (vertices[c] - vertices[a]));

					normals[a] += _normal;
					normals[b] += _normal;
					normals[c] += _normal;
				}
			}
			else
			{
				// It's just a list of triangles, so generate face normals
				for (u32 i = 0; i < numVertices; i += 3)
				{
					Maths::Vector3 &a = vertices[i];
					Maths::Vector3 &b = vertices[i + 1];
					Maths::Vector3 &c = vertices[i + 2];

					const Maths::Vector3 _normal = Maths::Vector3::Cross(b - a, c - a);

					normals[i] = _normal;
					normals[i + 1] = _normal;
					normals[i + 2] = _normal;
				}
			}

			for (u32 i = 0; i < numVertices; ++i)
			{
				normals[i].Normalize();
			}

			return normals;
		}

		Maths::Vector3* Mesh::GenerateTangents(u32 numVertices, Maths::Vector3* vertices, u32* indices, u32 numIndices, Maths::Vector2* texCoords)
		{
			if (!texCoords)
			{
				return nullptr;
			}

			Maths::Vector3* tangents = new Maths::Vector3[numVertices];

			for (u32 i = 0; i < numVertices; ++i)
			{
				tangents[i] = Maths::Vector3();
			}

			if (indices)
			{
				for (u32 i = 0; i < numIndices; i += 3)
				{
					int a = indices[i];
					int b = indices[i + 1];
					int c = indices[i + 2];

					const Maths::Vector3 tangent =
						GenerateTangent(vertices[a], vertices[b], vertices[c], texCoords[a], texCoords[b], texCoords[c]);

					tangents[a] += tangent;
					tangents[b] += tangent;
					tangents[c] += tangent;
				}
			}
			else
			{
				for (u32 i = 0; i < numVertices; i += 3)
				{
					const Maths::Vector3 tangent = GenerateTangent(vertices[i], vertices[i + 1], vertices[i + 2], texCoords[i], texCoords[i + 1],
						texCoords[i + 2]);

					tangents[i] += tangent;
					tangents[i + 1] += tangent;
					tangents[i + 2] += tangent;
				}
			}
			for (u32 i = 0; i < numVertices; ++i)
			{
				tangents[i].Normalize();
			}

			return tangents;
		}
	}
}
