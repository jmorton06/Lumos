#include "Precompiled.h"
#include "Graphics/Model.h"
#include "Graphics/Mesh.h"
#include "Graphics/Material.h"
#include "Core/OS/FileSystem.h"

#include "Graphics/API/Texture.h"
#include "Maths/Maths.h"

#include "Maths/Transform.h"
#include "Core/Application.h"
#include "Core/StringUtilities.h"

#include <OpenFBX/ofbx.h>

const uint32_t MAX_PATH_LENGTH = 260;

namespace Lumos::Graphics
{
	std::string m_FBXModelDirectory;
	
	enum class Orientation
	{
		Y_UP,
		Z_UP,
		Z_MINUS_UP,
		X_MINUS_UP,
		X_UP
	};
	
	Orientation orientation = Orientation::Y_UP;
	float fbx_scale = 1.f;
	
	static ofbx::Vec3 operator-(const ofbx::Vec3& a, const ofbx::Vec3& b)
	{
		return {a.x - b.x, a.y - b.y, a.z - b.z};
	}
	
	static ofbx::Vec2 operator-(const ofbx::Vec2& a, const ofbx::Vec2& b)
	{
		return {a.x - b.x, a.y - b.y};
	}
	
	Maths::Vector3 FixOrientation(const Maths::Vector3& v)
	{
		switch(orientation)
		{
			case Orientation::Y_UP:
			return Maths::Vector3(v.x, v.y, v.z);
			case Orientation::Z_UP:
			return Maths::Vector3(v.x, v.z, -v.y);
			case Orientation::Z_MINUS_UP:
			return Maths::Vector3(v.x, -v.z, v.y);
			case Orientation::X_MINUS_UP:
			return Maths::Vector3(v.y, -v.x, v.z);
			case Orientation::X_UP:
			return Maths::Vector3(-v.y, v.x, v.z);
		}
		return Maths::Vector3(v.x, v.y, v.z);
	}
	
	Maths::Quaternion FixOrientation(const Maths::Quaternion& v)
	{
		switch(orientation)
		{
			case Orientation::Y_UP:
			return Maths::Quaternion(v.x, v.y, v.z, v.w);
			case Orientation::Z_UP:
			return Maths::Quaternion(v.x, v.z, -v.y, v.w);
			case Orientation::Z_MINUS_UP:
			return Maths::Quaternion(v.x, -v.z, v.y, v.w);
			case Orientation::X_MINUS_UP:
			return Maths::Quaternion(v.y, -v.x, v.z, v.w);
			case Orientation::X_UP:
			return Maths::Quaternion(-v.y, v.x, v.z, v.w);
		}
		return Maths::Quaternion(v.x, v.y, v.z, v.w);
	}
	
	static void computeTangents(ofbx::Vec3* out, int vertex_count, const ofbx::Vec3* vertices, const ofbx::Vec3* normals, const ofbx::Vec2* uvs)
	{
		for(int i = 0; i < vertex_count; i += 3)
		{
			const ofbx::Vec3 v0 = vertices[i + 0];
			const ofbx::Vec3 v1 = vertices[i + 1];
			const ofbx::Vec3 v2 = vertices[i + 2];
			const ofbx::Vec2 uv0 = uvs[i + 0];
			const ofbx::Vec2 uv1 = uvs[i + 1];
			const ofbx::Vec2 uv2 = uvs[i + 2];
			
			const ofbx::Vec3 dv10 = v1 - v0;
			const ofbx::Vec3 dv20 = v2 - v0;
			const ofbx::Vec2 duv10 = uv1 - uv0;
			const ofbx::Vec2 duv20 = uv2 - uv0;
			
			const float dir = duv20.x * duv10.y - duv20.y * duv10.x < 0 ? -1.f : 1.f;
			ofbx::Vec3 tangent;
			tangent.x = (dv20.x * duv10.y - dv10.x * duv20.y) * dir;
			tangent.y = (dv20.y * duv10.y - dv10.y * duv20.y) * dir;
			tangent.z = (dv20.z * duv10.y - dv10.z * duv20.y) * dir;
			const float l = 1 / sqrtf(float(tangent.x * tangent.x + tangent.y * tangent.y + tangent.z * tangent.z));
			tangent.x *= l;
			tangent.y *= l;
			tangent.z *= l;
			out[i + 0] = tangent;
			out[i + 1] = tangent;
			out[i + 2] = tangent;
		}
	}
	
	Maths::Vector2 ToLumosVector(const ofbx::Vec2& vec)
	{
		return Maths::Vector2(float(vec.x), float(vec.y));
	}
	
	Maths::Vector3 ToLumosVector(const ofbx::Vec3& vec)
	{
		return Maths::Vector3(float(vec.x), float(vec.y), float(vec.z));
	}
	
	Maths::Vector4 ToLumosVector(const ofbx::Vec4& vec)
	{
		return Maths::Vector4(float(vec.x), float(vec.y), float(vec.z), float(vec.w));
	}
	
	Maths::Vector4 ToLumosVector(const ofbx::Color& vec)
	{
		return Maths::Vector4(float(vec.r), float(vec.g), float(vec.b), 1.0f);
	}
	
	Maths::Quaternion ToLumosQuat(const ofbx::Quat& quat)
	{
		return Maths::Quaternion(float(quat.x), float(quat.y), float(quat.z), float(quat.w));
	}
	
	void Model::LoadFBX(const std::string& path)
	{
		std::string err;
		std::string pathCopy = path;
		pathCopy = StringUtilities::BackSlashesToSlashes(pathCopy);
		m_FBXModelDirectory = pathCopy.substr(0, pathCopy.find_last_of('/'));
		
		std::string name = m_FBXModelDirectory.substr(m_FBXModelDirectory.find_last_of('/') + 1);
		
		std::string ext = StringUtilities::GetFilePathExtension(path);
		int64_t size = FileSystem::GetFileSize(path);
		auto data = FileSystem::ReadFile(path);
		
		if(data == nullptr)
		{
			LUMOS_LOG_WARN("Failed to load fbx file"); return;
		}
		const bool ignoreGeometry = false;
		const uint64_t flags = ignoreGeometry ? (uint64_t)ofbx::LoadFlags::IGNORE_GEOMETRY : (uint64_t)ofbx::LoadFlags::TRIANGULATE;
		
		ofbx::IScene* scene = ofbx::load(data, uint32_t(size), flags);
		
		err = ofbx::getError();
		
		if(!err.empty() || !scene)
		{
			LUMOS_LOG_CRITICAL(err);
		}
		
		const ofbx::GlobalSettings* settings = scene->getGlobalSettings();
		switch(settings->UpAxis)
		{
			case ofbx::UpVector_AxisX:
			orientation = Orientation::X_UP;
			break;
			case ofbx::UpVector_AxisY:
			orientation = Orientation::Y_UP;
			break;
			case ofbx::UpVector_AxisZ:
			orientation = Orientation::Z_UP;
			break;
		}
		
		int c = scene->getMeshCount();
		for(int i = 0; i < c; ++i)
		{
			const ofbx::Mesh* fbx_mesh = (const ofbx::Mesh*)scene->getMesh(i);
			auto geom = fbx_mesh->getGeometry();
			auto numIndices = geom->getIndexCount();
			int vertex_count = geom->getVertexCount();
			const ofbx::Vec3* vertices = geom->getVertices();
			const ofbx::Vec3* normals = geom->getNormals();
			const ofbx::Vec3* tangents = geom->getTangents();
			const ofbx::Vec4* colours = geom->getColors();
			const ofbx::Vec2* uvs = geom->getUVs();
			Graphics::Vertex* tempvertices = new Graphics::Vertex[vertex_count];
			uint32_t* indicesArray = new uint32_t[numIndices];
			
			Ref<Maths::BoundingBox> boundingBox = CreateRef<Maths::BoundingBox>();
			
			auto indices = geom->getFaceIndices();
			
			ofbx::Vec3* generatedTangents = nullptr;
			if(!tangents && normals && uvs)
			{
				generatedTangents = new ofbx::Vec3[vertex_count];
				computeTangents(generatedTangents, vertex_count, vertices, normals, uvs);
				tangents = generatedTangents;
			}
			
			for(int i = 0; i < vertex_count; ++i)
			{
				ofbx::Vec3 cp = vertices[i];
				
				auto& vertex = tempvertices[i];
				vertex.Position = Maths::Vector3(float(cp.x), float(cp.y), float(cp.z));
				FixOrientation(vertex.Position);
				boundingBox->Merge(vertex.Position);
				
				if(normals)
					vertex.Normal = Maths::Vector3(float(normals[i].x), float(normals[i].y), float(normals[i].z));
				if(uvs)
					vertex.TexCoords = Maths::Vector2(float(uvs[i].x), 1.0f - float(uvs[i].y));
				if(colours)
					vertex.Colours = Maths::Vector4(float(colours[i].x), float(colours[i].y), float(colours[i].z), float(colours[i].w));
				if(tangents)
					vertex.Tangent = Maths::Vector3(float(tangents[i].x), float(tangents[i].y), float(tangents[i].z));
				
				FixOrientation(vertex.Normal);
				FixOrientation(vertex.Tangent);
			}
			
			for(int i = 0; i < numIndices; i++)
			{
				int index = (i % 3 == 2) ? (-indices[i] - 1) : indices[i];
				
				indicesArray[i] = index;
			}
			
			Ref<Graphics::VertexBuffer> vb = Ref<Graphics::VertexBuffer>(Graphics::VertexBuffer::Create());
			vb->SetData(sizeof(Graphics::Vertex) * vertex_count, tempvertices);
    
			Ref<Graphics::IndexBuffer> ib;
			ib.reset(Graphics::IndexBuffer::Create(indicesArray, numIndices));
			
			Ref<Material> pbrMaterial = CreateRef<Material>();
			
			const ofbx::Material* material = fbx_mesh->getMaterialCount() > 0 ? fbx_mesh->getMaterial(0) : nullptr;
			if(material)
			{
				PBRMataterialTextures textures;
				Graphics::MaterialProperties properties;
				
				properties.albedoColour = ToLumosVector(material->getDiffuseColor());
				properties.metallicColour = ToLumosVector(material->getSpecularColor());
				
				float roughness = 1.0f - Maths::Sqrt(float(material->getShininess()) / 100.0f);
				properties.roughnessColour = Maths::Vector3(roughness);
				
				const ofbx::Texture* diffuseTexture = material->getTexture(ofbx::Texture::TextureType::DIFFUSE);
				if(diffuseTexture)
				{
					std::string stringFilepath;
					ofbx::DataView filename = diffuseTexture->getRelativeFileName();
					if(filename == "")
						filename = diffuseTexture->getFileName();
					
					char filePath[MAX_PATH_LENGTH];
					filename.toString(filePath);
					
					stringFilepath = std::string(filePath);
					stringFilepath = m_FBXModelDirectory + "/" + StringUtilities::BackSlashesToSlashes(stringFilepath);
					Graphics::Texture2D* texture2D = Graphics::Texture2D::CreateFromFile(stringFilepath, stringFilepath, Graphics::TextureParameters(Graphics::TextureFilter::LINEAR, Graphics::TextureFilter::LINEAR));
					if(texture2D)
						textures.albedo = (Ref<Graphics::Texture2D>(texture2D));
				}
				
				const ofbx::Texture* normalTexture = material->getTexture(ofbx::Texture::TextureType::NORMAL);
				if(normalTexture)
				{
					std::string stringFilepath;
					ofbx::DataView filename = normalTexture->getRelativeFileName();
					if(filename == "")
						filename = normalTexture->getFileName();
					
					char filePath[MAX_PATH_LENGTH];
					filename.toString(filePath);
					
					stringFilepath = std::string(filePath);
					stringFilepath = m_FBXModelDirectory + "/" + StringUtilities::BackSlashesToSlashes(stringFilepath);
					Graphics::Texture2D* texture2D = Graphics::Texture2D::CreateFromFile(stringFilepath, stringFilepath, Graphics::TextureParameters(Graphics::TextureFilter::LINEAR, Graphics::TextureFilter::LINEAR));
					if(texture2D)
						textures.normal = (Ref<Graphics::Texture2D>(texture2D));
				}
				
				const ofbx::Texture* specularTexture = material->getTexture(ofbx::Texture::TextureType::SPECULAR);
				if(specularTexture)
				{
					std::string stringFilepath;
					ofbx::DataView filename = specularTexture->getRelativeFileName();
					if(filename == "")
						filename = specularTexture->getFileName();
					
					char filePath[MAX_PATH_LENGTH];
					filename.toString(filePath);
					
					stringFilepath = std::string(filePath);
					stringFilepath = m_FBXModelDirectory + "/" + StringUtilities::BackSlashesToSlashes(stringFilepath);
					Graphics::Texture2D* texture2D = Graphics::Texture2D::CreateFromFile(stringFilepath, stringFilepath, Graphics::TextureParameters(Graphics::TextureFilter::LINEAR, Graphics::TextureFilter::LINEAR));
					if(texture2D)
						textures.metallic = (Ref<Graphics::Texture2D>(texture2D));
				}
				
				const ofbx::Texture* shininessTexture = material->getTexture(ofbx::Texture::TextureType::SHININESS);
				if(shininessTexture)
				{
					std::string stringFilepath;
					ofbx::DataView filename = shininessTexture->getRelativeFileName();
					if(filename == "")
						filename = shininessTexture->getFileName();
					
					char filePath[MAX_PATH_LENGTH];
					filename.toString(filePath);
					
					stringFilepath = std::string(filePath);
					stringFilepath = m_FBXModelDirectory + "/" +StringUtilities::BackSlashesToSlashes(stringFilepath);
					Graphics::Texture2D* texture2D = Graphics::Texture2D::CreateFromFile(stringFilepath, stringFilepath, Graphics::TextureParameters(Graphics::TextureFilter::LINEAR, Graphics::TextureFilter::LINEAR));
					if(texture2D)
						textures.roughness = (Ref<Graphics::Texture2D>(texture2D));
				}
				const ofbx::Texture* emissiveTexture = material->getTexture(ofbx::Texture::TextureType::EMISSIVE);
				if(emissiveTexture)
				{
					std::string stringFilepath;
					ofbx::DataView filename = emissiveTexture->getRelativeFileName();
					if(filename == "")
						filename = emissiveTexture->getFileName();
					
					char filePath[MAX_PATH_LENGTH];
					filename.toString(filePath);
					
					stringFilepath = std::string(filePath);
					stringFilepath = m_FBXModelDirectory + "/" + StringUtilities::BackSlashesToSlashes(stringFilepath);
					Graphics::Texture2D* texture2D = Graphics::Texture2D::CreateFromFile(stringFilepath, stringFilepath, Graphics::TextureParameters(Graphics::TextureFilter::LINEAR, Graphics::TextureFilter::LINEAR));
					if(texture2D)
						textures.emissive = (Ref<Graphics::Texture2D>(texture2D));
				}
				
				const ofbx::Texture* reflectionTexture = material->getTexture(ofbx::Texture::TextureType::REFLECTION);
				if(reflectionTexture)
				{
					std::string stringFilepath;
					ofbx::DataView filename = reflectionTexture->getRelativeFileName();
					if(filename == "")
						filename = reflectionTexture->getFileName();
					
					char filePath[MAX_PATH_LENGTH];
					filename.toString(filePath);
					
					stringFilepath = std::string(filePath);
					stringFilepath = m_FBXModelDirectory + "/" + StringUtilities::BackSlashesToSlashes(stringFilepath);
					Graphics::Texture2D* texture2D = Graphics::Texture2D::CreateFromFile(stringFilepath, stringFilepath, Graphics::TextureParameters(Graphics::TextureFilter::LINEAR, Graphics::TextureFilter::LINEAR));
					if(texture2D)
						textures.metallic = (Ref<Graphics::Texture2D>(texture2D));
				}
				
				pbrMaterial->SetTextures(textures);
				pbrMaterial->SetMaterialProperites(properties);
			}
			
			auto mesh = CreateRef<Graphics::Mesh>(vb, ib, boundingBox);
			if(c == 1)
			{
				mesh->SetName(fbx_mesh->name);
				if(material)
					mesh->SetMaterial(pbrMaterial);
				
				m_Meshes.push_back(mesh);
				
				auto transform = Maths::Transform();
				
				auto object = fbx_mesh;
				ofbx::Vec3 p = object->getLocalTranslation();
				
				const Maths::Matrix3 gInvert = Maths::Matrix3(-1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 1.0, 0.0);
				Maths::Vector3 pos = (Maths::Vector3(static_cast<float>(p.x), static_cast<float>(p.y), static_cast<float>(p.z)));
				transform.SetLocalPosition(FixOrientation(pos));
				
				ofbx::Vec3 r = object->getLocalRotation();
				Maths::Vector3 rot = FixOrientation(Maths::Vector3(static_cast<float>(r.x), static_cast<float>(r.y), static_cast<float>(r.z)));
				transform.SetLocalOrientation(Maths::Quaternion::EulerAnglesToQuaternion(rot.x, rot.y, rot.z));
				
				ofbx::Vec3 s = object->getLocalScaling();
				Maths::Vector3 scl = Maths::Vector3(static_cast<float>(s.x), static_cast<float>(s.y), static_cast<float>(s.z));
				transform.SetLocalScale(scl);
			}
			else
			{
				mesh->SetName(fbx_mesh->name);
				if(material)
					mesh->SetMaterial(pbrMaterial);
				
				m_Meshes.push_back(mesh);
				auto transform = Maths::Transform();
				
				auto object = fbx_mesh;
				ofbx::Vec3 p = object->getLocalTranslation();
				const Maths::Matrix3 gInvert = Maths::Matrix3(-1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 1.0, 0.0);
				Maths::Vector3 pos = (Maths::Vector3(static_cast<float>(p.x), static_cast<float>(p.y), static_cast<float>(p.z))); // * settings->customScale());
				transform.SetLocalPosition(FixOrientation(pos));
				
				ofbx::Vec3 r = object->getLocalRotation();
				Maths::Vector3 rot = FixOrientation(Maths::Vector3(static_cast<float>(r.x), static_cast<float>(r.y), static_cast<float>(r.z)));
				transform.SetLocalOrientation(Maths::Quaternion::EulerAnglesToQuaternion(rot.x, rot.y, rot.z));
				
				ofbx::Vec3 s = object->getLocalScaling();
				Maths::Vector3 scl = Maths::Vector3(static_cast<float>(s.x), static_cast<float>(s.y), static_cast<float>(s.z));
				transform.SetLocalScale(scl);
			}
			
			if(generatedTangents)
				delete[] generatedTangents;
			delete[] tempvertices;
			delete[] indicesArray;
		}
	}
	
}
