#include "lmpch.h"
#include "ModelLoader.h"
#include "Graphics/Mesh.h"
#include "Graphics/Material.h"
#include "Maths/Transform.h"
#include "ECS/Component/MeshComponent.h"
#include "ECS/Component/MaterialComponent.h"
#include "Graphics/API/Texture.h"
#include "Maths/Maths.h"

#include "App/Application.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobjloader/tiny_obj_loader.h>

namespace Lumos
{
	std::string m_Directory;
	std::vector<Ref<Graphics::Texture2D>> m_Textures;

	Ref<Graphics::Texture2D> LoadMaterialTextures(const std::string& typeName, std::vector<Ref<Graphics::Texture2D>>& textures_loaded, const std::string& name, const std::string& directory, Graphics::TextureParameters format)
	{
		for(u32 j = 0; j < textures_loaded.size(); j++)
		{
			if(std::strcmp(textures_loaded[j]->GetFilepath().c_str(), (directory + "/" + name).c_str()) == 0)
			{
				return textures_loaded[j];
			}
		}

		{ // If texture hasn't been loaded already, load it
			Graphics::TextureLoadOptions options(false, true);
			auto texture = Ref<Graphics::Texture2D>(Graphics::Texture2D::CreateFromFile(typeName, directory + "/" + name, format, options));
			textures_loaded.push_back(texture); // Store it as texture loaded for entire model, to ensure we won't unnecessary load duplicate textures.

			return texture;
		}
	}

	entt::entity ModelLoader::LoadOBJ(const std::string& path, entt::registry& registry)
	{
		std::string resolvedPath = path;
		tinyobj::attrib_t attrib;
		std::string error;

		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;

		m_Directory = resolvedPath.substr(0, resolvedPath.find_last_of('/'));

		std::string name = m_Directory.substr(m_Directory.find_last_of('/') + 1);

		bool ok = tinyobj::LoadObj(
			&attrib, &shapes, &materials, &error, (resolvedPath).c_str(), (m_Directory + "/").c_str());

		if(!ok)
		{
			LUMOS_LOG_CRITICAL(error);
		}

		auto entity = registry.create();
		registry.emplace<NameComponent>(entity, name);
		bool singleMesh = shapes.size() == 1;

		for(const auto& shape : shapes)
		{
			u32 vertexCount = 0;
			const u32 numIndices = static_cast<u32>(shape.mesh.indices.size());
			const u32 numVertices = numIndices; // attrib.vertices.size();// numIndices / 3.0f;
			Graphics::Vertex* vertices = lmnew Graphics::Vertex[numVertices];
			u32* indices = lmnew u32[numIndices];

			std::unordered_map<Graphics::Vertex, uint32_t> uniqueVertices;

			Ref<Maths::BoundingBox> boundingBox = CreateRef<Maths::BoundingBox>();

			for(u32 i = 0; i < shape.mesh.indices.size(); i++)
			{
				auto& index = shape.mesh.indices[i];
				Graphics::Vertex vertex;

				if(!attrib.texcoords.empty())
				{
					vertex.TexCoords = (Maths::Vector2(
						attrib.texcoords[2 * index.texcoord_index + 0],
						1.0f - attrib.texcoords[2 * index.texcoord_index + 1]));
				}
				else
				{
					vertex.TexCoords =
						Maths::Vector2(0.0f, 0.0f);
				}
				vertex.Position = (Maths::Vector3(
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]));

				boundingBox->Merge(vertex.Position);

				if(!attrib.normals.empty())
				{
					vertex.Normal = (Maths::Vector3(
						attrib.normals[3 * index.normal_index + 0],
						attrib.normals[3 * index.normal_index + 1],
						attrib.normals[3 * index.normal_index + 2]));
				}
				else
				{
					vertex.Normal = (Maths::Vector3(
						0.0f,
						0.0f,
						0.0f));
				}

				Maths::Vector4 colour = Maths::Vector4(0.0f);

				if(shape.mesh.material_ids[0] >= 0)
				{
					tinyobj::material_t* mp = &materials[shape.mesh.material_ids[0]];
					colour = Maths::Vector4(mp->diffuse[0], mp->diffuse[1], mp->diffuse[2], 1.0f);
				}

				vertex.Colours = colour;

				if(uniqueVertices.count(vertex) == 0)
				{
					uniqueVertices[vertex] = static_cast<uint32_t>(vertexCount);
					vertices[vertexCount] = vertex;
				}

				indices[vertexCount] = uniqueVertices[vertex];

				vertexCount++;
			}

			Ref<Material> pbrMaterial = CreateRef<Material>();

			PBRMataterialTextures textures;

			if(shape.mesh.material_ids[0] >= 0)
			{
				tinyobj::material_t* mp = &materials[shape.mesh.material_ids[0]];

				if(mp->diffuse_texname.length() > 0)
				{
					Ref<Graphics::Texture2D> texture = LoadMaterialTextures("Albedo", m_Textures, mp->diffuse_texname, m_Directory, Graphics::TextureParameters(Graphics::TextureFilter::NEAREST, Graphics::TextureFilter::NEAREST, mp->diffuse_texopt.clamp ? Graphics::TextureWrap::CLAMP_TO_EDGE : Graphics::TextureWrap::REPEAT));
					if(texture)
						textures.albedo = texture;
				}

				if(mp->bump_texname.length() > 0)
				{
					Ref<Graphics::Texture2D> texture = LoadMaterialTextures("Normal", m_Textures, mp->bump_texname, m_Directory, Graphics::TextureParameters(Graphics::TextureFilter::NEAREST, Graphics::TextureFilter::NEAREST, mp->bump_texopt.clamp ? Graphics::TextureWrap::CLAMP_TO_EDGE : Graphics::TextureWrap::REPEAT));
					if(texture)
						textures.normal = texture; //pbrMaterial->SetNormalMap(texture);
				}

				if(mp->roughness_texname.length() > 0)
				{
					Ref<Graphics::Texture2D> texture = LoadMaterialTextures("Roughness", m_Textures, mp->roughness_texname.c_str(), m_Directory, Graphics::TextureParameters(Graphics::TextureFilter::NEAREST, Graphics::TextureFilter::NEAREST, mp->roughness_texopt.clamp ? Graphics::TextureWrap::CLAMP_TO_EDGE : Graphics::TextureWrap::REPEAT));
					if(texture)
						textures.roughness = texture;
				}

				if(mp->metallic_texname.length() > 0)
				{
					Ref<Graphics::Texture2D> texture = LoadMaterialTextures("Metallic", m_Textures, mp->metallic_texname, m_Directory, Graphics::TextureParameters(Graphics::TextureFilter::NEAREST, Graphics::TextureFilter::NEAREST, mp->metallic_texopt.clamp ? Graphics::TextureWrap::CLAMP_TO_EDGE : Graphics::TextureWrap::REPEAT));
					if(texture)
						textures.metallic = texture;
				}

				if(mp->specular_highlight_texname.length() > 0)
				{
					Ref<Graphics::Texture2D> texture = LoadMaterialTextures("Metallic", m_Textures, mp->specular_highlight_texname, m_Directory, Graphics::TextureParameters(Graphics::TextureFilter::NEAREST, Graphics::TextureFilter::NEAREST, mp->specular_texopt.clamp ? Graphics::TextureWrap::CLAMP_TO_EDGE : Graphics::TextureWrap::REPEAT));
					if(texture)
						textures.metallic = texture;
				}
			}

			pbrMaterial->SetTextures(textures);

			Ref<Graphics::VertexArray> va;
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

			Ref<Graphics::IndexBuffer> ib;
			ib.reset(Graphics::IndexBuffer::Create(indices, numIndices));

			auto mesh = CreateRef<Graphics::Mesh>(va, ib, boundingBox);
			if(singleMesh)
			{
				registry.emplace<MeshComponent>(entity, mesh);
				registry.emplace<MaterialComponent>(entity, pbrMaterial);
				if(!registry.has<Maths::Transform>(entity))
					registry.emplace<Maths::Transform>(entity);
			}
			else
			{
				auto meshEntity = registry.create();
				registry.emplace<MeshComponent>(meshEntity, mesh);
				registry.emplace<MaterialComponent>(meshEntity, pbrMaterial);
				registry.emplace<Maths::Transform>(meshEntity);
				registry.emplace<Hierarchy>(meshEntity, entity);
			}

			delete[] vertices;
			delete[] indices;
		}

		return entity;
	}

}
