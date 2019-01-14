#include "LM.h"
#include "Mesh.h"
#include "Material.h"
#include "API/Renderer.h"
#include "Platform/GraphicsAPI/Vulkan/VKDevice.h"
#include "Maths/BoundingSphere.h"

namespace Lumos
{

	Mesh::Mesh() : m_VertexArray(nullptr), m_IndexBuffer(nullptr), m_Material(nullptr), m_ArrayCleanUp(false), m_TextureCleanUp(false), m_BoundingSphere(nullptr)
	{
		Init();
	}

	Mesh::Mesh(const Mesh& mesh)
		: m_VertexArray(mesh.m_VertexArray), m_IndexBuffer(mesh.m_IndexBuffer), m_Material(mesh.m_Material), m_ArrayCleanUp(false), m_TextureCleanUp(false), m_BoundingSphere(mesh.m_BoundingSphere)
	{
		Init();
	}

	Mesh::Mesh(std::shared_ptr<VertexArray>& vertexArray, std::shared_ptr<IndexBuffer>& indexBuffer, const std::shared_ptr<Material>& material, const std::shared_ptr<maths::BoundingSphere>& boundingSphere)
		: m_VertexArray(vertexArray), m_IndexBuffer(indexBuffer), m_Material(material), m_ArrayCleanUp(true), m_TextureCleanUp(false), m_BoundingSphere(boundingSphere)
	{
		Init();
	}

	Mesh::~Mesh()
	{
		for (auto& cmdBuffer : m_CMDBuffers)
			delete cmdBuffer;
	}

	void Mesh::Draw()
	{
		//if (m_Material)
		//	m_Material->Bind();
		m_VertexArray->Bind();
		m_IndexBuffer->Bind();
		Renderer::Draw(DrawType::TRIANGLE, m_IndexBuffer->GetCount());
		m_IndexBuffer->Unbind();
		m_VertexArray->Unbind();
		//if (m_Material)
		//	m_Material->Unbind();
	}

	void Mesh::Draw(bool bindMaterial)
	{
		m_VertexArray->Bind();
		m_IndexBuffer->Bind();
		Renderer::Draw(DrawType::TRIANGLE, m_IndexBuffer->GetCount());
		m_IndexBuffer->Unbind();
		m_VertexArray->Unbind();
	}

	void Mesh::Init()
	{
        for (auto& cmdBuffer : m_CMDBuffers)
            delete cmdBuffer;

		m_CMDBuffers.resize(2);// graphics::VKDevice::Instance()->m_SwapChainSize);

		for (auto& m_CMDBuffer : m_CMDBuffers)
		{
			m_CMDBuffer = graphics::api::CommandBuffer::Create();
			m_CMDBuffer->Init(false);
		}
	}

	maths::Vector3 Mesh::GenerateTangent(const maths::Vector3 &a, const maths::Vector3 &b, const maths::Vector3 &c, const maths::Vector2 &ta, const maths::Vector2 &tb, const maths::Vector2 &tc)
	{
		const maths::Vector2 coord1 = tb - ta;
		const maths::Vector2 coord2 = tc - ta;

		const maths::Vector3 vertex1 = b - a;
		const maths::Vector3 vertex2 = c - a;

		const maths::Vector3 axis = maths::Vector3(vertex1*coord2.GetY() - vertex2*coord1.GetY());

		const float factor = 1.0f / (coord1.GetX() * coord2.GetY() - coord2.GetX() * coord1.GetY());

		return axis * factor;
	}

	maths::Vector3* Mesh::GenerateNormals(uint numVertices, maths::Vector3* vertices, uint* indices, uint numIndices)
	{
		maths::Vector3* normals = new maths::Vector3[numVertices];

		for (uint i = 0; i < numVertices; ++i)
		{
			normals[i] = maths::Vector3();
		}

		if (indices)
		{
			int test = 0;
			for (uint i = 0; i < numIndices; i += 3)
			{
				const int a = indices[i];
				const int b = indices[i + 1];
				const int c = indices[i + 2];

				const maths::Vector3 _normal = maths::Vector3::Cross((vertices[b] - vertices[a]), (vertices[c] - vertices[a]));

				normals[a] += _normal;
				normals[b] += _normal;
				normals[c] += _normal;

				test += 3;
			}
		}
		else
		{
			// It's just a list of triangles, so generate face normals
			for (uint i = 0; i < numVertices; i += 3)
			{
				maths::Vector3 &a = vertices[i];
				maths::Vector3 &b = vertices[i + 1];
				maths::Vector3 &c = vertices[i + 2];

				const maths::Vector3 _normal = maths::Vector3::Cross(b - a, c - a);

				normals[i] = _normal;
				normals[i + 1] = _normal;
				normals[i + 2] = _normal;
			}
		}

		for (uint i = 0; i < numVertices; ++i)
		{
			normals[i].Normalise();
		}

		return normals;
	}

	maths::Vector3* Mesh::GenerateTangents(uint numVertices, maths::Vector3* vertices, uint* indices, uint numIndices, maths::Vector2* texCoords)
	{
		if (!texCoords)
		{
			return nullptr;
		}

		maths::Vector3* tangents = new maths::Vector3[numVertices];

		for (uint i = 0; i < numVertices; ++i)
		{
			tangents[i] = maths::Vector3();
		}

		if (indices)
		{
			for (uint i = 0; i < numIndices; i += 3)
			{
				int a = indices[i];
				int b = indices[i + 1];
				int c = indices[i + 2];

				const maths::Vector3 tangent =
					GenerateTangent(vertices[a], vertices[b], vertices[c], texCoords[a], texCoords[b], texCoords[c]);

				tangents[a] += tangent;
				tangents[b] += tangent;
				tangents[c] += tangent;
			}
		}
		else
		{
			for (uint i = 0; i < numVertices; i += 3)
			{
				const maths::Vector3 tangent = GenerateTangent(vertices[i], vertices[i + 1], vertices[i + 2], texCoords[i], texCoords[i + 1],
					texCoords[i + 2]);

				tangents[i] += tangent;
				tangents[i + 1] += tangent;
				tangents[i + 2] += tangent;
			}
		}
		for (uint i = 0; i < numVertices; ++i)
		{
			tangents[i].Normalise();
		}

		return tangents;
	}
}
