#include "LM.h"
#include "ModelLoader.h"
#include "Graphics/Mesh.h"
#include "Graphics/Material.h"
#include "Entity/Entity.h"
#include "Entity/Component/MeshComponent.h"
#include "Graphics/API/Textures/Texture2D.h"
#include "Maths/BoundingSphere.h"
#include "Utilities/AssetsManager.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobjloader/tiny_obj_loader.h>

namespace Lumos
{
	String m_Directory;
	std::vector<std::shared_ptr<Texture2D>> m_Textures;

	std::shared_ptr<Texture2D> LoadMaterialTextures(const String& typeName, std::vector<std::shared_ptr<Texture2D>>& textures_loaded, const String& name, const String& directory, TextureParameters format)
	{
		for (uint j = 0; j < textures_loaded.size(); j++)
		{
			if (std::strcmp(textures_loaded[j]->GetFilepath().c_str(), (directory + "/" + name).c_str()) == 0)
			{
				return textures_loaded[j];
			}
		}

		{   // If texture hasn't been loaded already, load it
			TextureLoadOptions options(false, true);
			auto texture = std::shared_ptr<Texture2D>(Texture2D::CreateFromFile(typeName, directory + "/" + name, format, options));
			textures_loaded.push_back(texture);  // Store it as texture loaded for entire model, to ensure we won't unnecessary load duplicate textures.

			return texture;
		}
	}

	std::shared_ptr<Entity> ModelLoader::LoadOBJ(const String& path)
	{
		String resolvedPath = path;
		tinyobj::attrib_t attrib;
		std::string error;

		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;

		m_Directory = resolvedPath.substr(0, resolvedPath.find_last_of('/'));
        
        String name = m_Directory.substr(m_Directory.find_last_of('/') + 1);

		bool ok = tinyobj::LoadObj(
			&attrib, &shapes, &materials, &error,
			(resolvedPath).c_str(),
			(m_Directory + "/").c_str()
		);

		if (!ok)
		{
			LUMOS_CORE_ERROR(error);
		}

		auto entity = std::make_shared<Entity>(name, nullptr);

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

				maths::Vector4 colour = maths::Vector4(0.0f);

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
					std::shared_ptr<Texture2D> texture = LoadMaterialTextures("Albedo", m_Textures, mp->diffuse_texname, m_Directory, TextureParameters(TextureFilter::NEAREST, TextureWrap::CLAMP_TO_EDGE));
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
					std::shared_ptr<Texture2D> texture = LoadMaterialTextures("Metallic", m_Textures, mp->ambient_texname.c_str(), m_Directory, TextureParameters(TextureWrap::CLAMP));
					//if(texture)// TODO: Fix or check if mesh mtl wrong
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

			auto meshEntity = std::make_shared<Entity>(shape.name, nullptr);
            auto mesh = std::make_shared<Mesh>(va, ib, pbrMaterial, boundingBox);
			meshEntity->AddComponent(std::make_unique<MeshComponent>(mesh));
			meshEntity->AddComponent(std::make_unique<TransformComponent>(maths::Matrix4()));
			entity->AddChildObject(meshEntity);

			delete[] vertices;
			delete[] indices;
		}

		return entity;
	}


}
