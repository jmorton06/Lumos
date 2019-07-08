#include "LM.h"
#include "ModelLoader.h"
#include "Graphics/Mesh.h"
#include "Graphics/Material.h"
#include "Entity/Entity.h"
#include "Entity/ECS.h"
#include "Entity/Component/MeshComponent.h"
#include "Graphics/API/Textures/Texture2D.h"
#include "Maths/BoundingSphere.h"
#include "Utilities/AssetsManager.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobjloader/tiny_obj_loader.h>

namespace Lumos
{
	String m_Directory;
	std::vector<std::shared_ptr<Graphics::Texture2D>> m_Textures;

	std::shared_ptr<Graphics::Texture2D> LoadMaterialTextures(const String& typeName, std::vector<std::shared_ptr<Graphics::Texture2D>>& textures_loaded, const String& name, const String& directory, Graphics::TextureParameters format)
	{
		for (u32 j = 0; j < textures_loaded.size(); j++)
		{
			if (std::strcmp(textures_loaded[j]->GetFilepath().c_str(), (directory + "/" + name).c_str()) == 0)
			{
				return textures_loaded[j];
			}
		}

		{   // If texture hasn't been loaded already, load it
			Graphics::TextureLoadOptions options(false, true);
			auto texture = std::shared_ptr<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(typeName, directory + "/" + name, format, options));
			textures_loaded.push_back(texture);  // Store it as texture loaded for entire model, to ensure we won't unnecessary load duplicate textures.

			return texture;
		}
	}

	Entity* ModelLoader::LoadOBJ(const String& path)
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

		auto entity = ECS::Instance()->CreateEntity(name);

		for (const auto& shape : shapes)
		{
			u32 vertexCount = 0;
			const u32 numIndices = static_cast<u32>(shape.mesh.indices.size());
			const u32 numVertices = numIndices;// attrib.vertices.size();// numIndices / 3.0f;
			Graphics::Vertex* vertices = new Graphics::Vertex[numVertices];
			u32* indices = new u32[numIndices];

			std::unordered_map<Graphics::Vertex, uint32_t> uniqueVertices;

			std::shared_ptr<Maths::BoundingSphere> boundingBox = std::make_shared<Maths::BoundingSphere>();

			for (u32 i = 0; i < shape.mesh.indices.size(); i++)
			{
				auto& index = shape.mesh.indices[i];
				Graphics::Vertex vertex;

				if (!attrib.texcoords.empty()) {
					vertex.TexCoords = (
							Maths::Vector2(
							attrib.texcoords[2 * index.texcoord_index + 0],
							1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
						)
						);
				}
				else
				{
					vertex.TexCoords =
							Maths::Vector2(0.0f, 0.0f);
				}
				vertex.Position = (
						Maths::Vector3(
						attrib.vertices[3 * index.vertex_index + 0],
						attrib.vertices[3 * index.vertex_index + 1],
						attrib.vertices[3 * index.vertex_index + 2]
					)
					);
                
				boundingBox->ExpandToFit(vertex.Position);

				if (!attrib.normals.empty())
				{
					vertex.Normal = (
							Maths::Vector3(
							attrib.normals[3 * index.normal_index + 0],
							attrib.normals[3 * index.normal_index + 1],
							attrib.normals[3 * index.normal_index + 2]
						)
						);
				}
				else
				{
					vertex.Normal = (
							Maths::Vector3(
							0.0f,
							0.0f,
							0.0f
						)
						);
				}

				Maths::Vector4 colour = Maths::Vector4(0.0f);

				if (shape.mesh.material_ids[0] >= 0)
				{
					tinyobj::material_t* mp = &materials[shape.mesh.material_ids[0]];
					colour = Maths::Vector4(mp->diffuse[0], mp->diffuse[1], mp->diffuse[2], 1.0f);
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
					std::shared_ptr<Graphics::Texture2D> texture = LoadMaterialTextures("Albedo", m_Textures, mp->diffuse_texname, m_Directory, Graphics::TextureParameters(Graphics::TextureFilter::NEAREST, Graphics::TextureWrap::CLAMP_TO_EDGE));
					if (texture)
						textures.albedo = texture;
				}

				if (mp->bump_texname.length() > 0)
				{
					std::shared_ptr<Graphics::Texture2D> texture = LoadMaterialTextures("Normal", m_Textures, mp->bump_texname, m_Directory, Graphics::TextureParameters(Graphics::TextureWrap::CLAMP));
					if (texture)
						textures.normal = texture;//pbrMaterial->SetNormalMap(texture);
				}

				if (mp->ambient_texname.length() > 0)
				{
					std::shared_ptr<Graphics::Texture2D> texture = LoadMaterialTextures("Metallic", m_Textures, mp->ambient_texname.c_str(), m_Directory, Graphics::TextureParameters(Graphics::TextureWrap::CLAMP));
					//if(texture)// TODO: Fix or check if mesh mtl wrong
					//	pbrMaterial->SetGlossMap(texture);
				}

				if (mp->specular_highlight_texname.length() > 0)
				{
					std::shared_ptr<Graphics::Texture2D> texture = LoadMaterialTextures("Specular", m_Textures, mp->specular_highlight_texname, m_Directory, Graphics::TextureParameters(Graphics::TextureWrap::CLAMP));
					if (texture)
						textures.roughness = texture;//pbrMaterial->SetSpecularMap(texture);
				}
			}

			pbrMaterial->SetTextures(textures);

			std::shared_ptr<Graphics::VertexArray> va;
			va.reset(Graphics::VertexArray::Create());

			Graphics::VertexBuffer* buffer = Graphics::VertexBuffer::Create(Graphics::BufferUsage::STATIC);
			buffer->SetData(sizeof(Graphics::Vertex) * numVertices, vertices);

			Graphics::BufferLayout layout;
			layout.Push<Maths::Vector3>("position");
			layout.Push<Maths::Vector4>("colour");
			layout.Push<Maths::Vector2>("texCoord");
			layout.Push<Maths::Vector3>("normal");
			layout.Push<Maths::Vector3>("tangent");
			buffer->SetLayout(layout);

			va->PushBuffer(buffer);

			std::shared_ptr<Graphics::IndexBuffer> ib;
			ib.reset(Graphics::IndexBuffer::Create(indices, numIndices));// / sizeof(u32));

			auto meshEntity = ECS::Instance()->CreateEntity(shape.name);
            auto mesh = std::make_shared<Graphics::Mesh>(va, ib, boundingBox);
			meshEntity->AddComponent<MeshComponent>(mesh);
			meshEntity->AddComponent<MaterialComponent>(pbrMaterial);
			meshEntity->AddComponent<TransformComponent>();
			entity->AddChild(meshEntity);

			delete[] vertices;
			delete[] indices;
		}

		return entity;
	}


}
