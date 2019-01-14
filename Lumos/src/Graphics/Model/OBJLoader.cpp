#include "LM.h"
#include "Model.h"
#include "Graphics/API/Textures/Texture2D.h"
#include "Maths/BoundingSphere.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "external/tinyobjloader/tiny_obj_loader.h"

#include "Utilities/AssetsManager.h"


namespace Lumos
{
	void Model::LoadOBJ(const String& path)
	{
		String resolvedPath = path;
		tinyobj::attrib_t attrib;
		std::string error;

		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;

		bool ok = tinyobj::LoadObj(
			&attrib, &shapes, &materials, &error,
			(resolvedPath).c_str(),
			(m_Directory + "/").c_str()
		);

		if (!ok)
		{
			LUMOS_CORE_ERROR(error);
		}

		for (const auto& shape : shapes)
		{
			uint vertexCount = 0;
			const uint numIndices = static_cast<uint>(shape.mesh.indices.size());
			const uint numVertices = numIndices;// attrib.vertices.size();// numIndices / 3.0f;
			Vertex* vertices = new Vertex[numVertices];
			uint* indices = new uint[numIndices];

			std::unordered_map<Vertex, uint32_t> uniqueVertices;

			std::shared_ptr<maths::BoundingSphere> boundingBox = std::make_shared<maths::BoundingSphere>();

			for (uint i = 0; i < shape.mesh.indices.size(); i++)
			{
				auto& index = shape.mesh.indices[i];
				Vertex vertex;

				if (!attrib.texcoords.empty()) {
					vertex.TexCoords = (
							maths::Vector2(
							attrib.texcoords[2 * index.texcoord_index + 0],
							1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
						)
						);
				}
				else
				{
					vertex.TexCoords =
							maths::Vector2(0.0f, 0.0f);
				}
				vertex.Position = (
						maths::Vector3(
						attrib.vertices[3 * index.vertex_index + 0],
						attrib.vertices[3 * index.vertex_index + 1],
						attrib.vertices[3 * index.vertex_index + 2]
					)
					);
				boundingBox->ExpandToFit(vertex.Position);

				if (!attrib.normals.empty())
				{
					vertex.Normal = (
							maths::Vector3(
							attrib.normals[3 * index.normal_index + 0],
							attrib.normals[3 * index.normal_index + 1],
							attrib.normals[3 * index.normal_index + 2]
						)
						);
				}
				else
				{
					vertex.Normal = (
							maths::Vector3(
							0.0f,
							0.0f,
							0.0f
						)
						);
				}

				maths::Vector4 colour = maths::Vector4(1.0f);

				if (shape.mesh.material_ids[0] >= 0)
				{
					tinyobj::material_t* mp = &materials[shape.mesh.material_ids[0]];
					colour = maths::Vector4(mp->diffuse[0], mp->diffuse[1], mp->diffuse[2], 1.0f);
				}

				vertex.Colours = colour;

				if (uniqueVertices.count(vertex) == 0)
				{
					uniqueVertices[vertex] = static_cast<uint32_t>(vertexCount);
					vertices[vertexCount] = vertex;
				}

				indices[vertexCount] = uniqueVertices[vertex];

				vertexCount++;
			}

			std::shared_ptr<Material> pbrMaterial = std::make_shared<Material>();

			PBRMataterialTextures textures;

			if (shape.mesh.material_ids[0] >= 0)
			{
				tinyobj::material_t* mp = &materials[shape.mesh.material_ids[0]];

				if (mp->diffuse_texname.length() > 0)
				{
					std::shared_ptr<Texture2D> texture = LoadMaterialTextures("Albedo", m_Textures, mp->diffuse_texname, m_Directory, TextureParameters(TextureWrap::REPEAT));
					if (texture)
						textures.albedo = texture;
				}

				if (mp->bump_texname.length() > 0)
				{
					std::shared_ptr<Texture2D> texture = LoadMaterialTextures("Normal", m_Textures, mp->bump_texname, m_Directory, TextureParameters(TextureWrap::CLAMP));
					if (texture)
						textures.normal = texture;//pbrMaterial->SetNormalMap(texture);
				}

				if (mp->ambient_texname.length() > 0)
				{
					//std::shared_ptr<Texture2D> texture = LoadMaterialTextures("Metallic", m_Textures, mp->ambient_texname.c_str(), m_Directory, TextureParameters(TextureWrap::CLAMP));
					//if(texture) TODO: Fix or check if mesh mtl wrong
					//	pbrMaterial->SetGlossMap(texture);
				}

				if (mp->specular_highlight_texname.length() > 0)
				{
					std::shared_ptr<Texture2D> texture = LoadMaterialTextures("Specular", m_Textures, mp->specular_highlight_texname, m_Directory, TextureParameters(TextureWrap::CLAMP));
					if (texture)
						textures.roughness = texture;//pbrMaterial->SetSpecularMap(texture);
				}
			}

			pbrMaterial->SetTextures(textures);

			std::shared_ptr<VertexArray> va;
			va.reset(VertexArray::Create());

			VertexBuffer* buffer = VertexBuffer::Create(BufferUsage::STATIC);
			buffer->SetData(sizeof(Vertex) * numVertices, vertices);

			graphics::BufferLayout layout;
			layout.Push<maths::Vector3>("position");
			layout.Push<maths::Vector4>("colour");
			layout.Push<maths::Vector2>("texCoord");
			layout.Push<maths::Vector3>("normal");
			layout.Push<maths::Vector3>("tangent");
			buffer->SetLayout(layout);

			va->PushBuffer(buffer);

			std::shared_ptr<IndexBuffer> ib;
			ib.reset(IndexBuffer::Create(indices, numIndices));// / sizeof(uint));

			m_Meshes.push_back(std::make_shared<Mesh>(va, ib, pbrMaterial,boundingBox));

			delete[] vertices;
			delete[] indices;
		}
	}
}
