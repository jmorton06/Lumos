#include "LM.h"
#include "Model.h"
#include "Graphics/API/Textures/Texture2D.h"
#include "System/VFS.h"
#include "../Material.h"
#include "App/SceneManager.h"
#include "../Mesh.h"
#include "../API/Shader.h"
#include "Utilities/AssetsManager.h"

namespace Lumos
{
	std::shared_ptr<Texture2D> Model::LoadMaterialTextures(const String& typeName, std::vector<std::shared_ptr<Texture2D>>& textures_loaded, const String& name, const String& directory, TextureParameters format)
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

	Model::Model() : m_CleanUpMesh(false)
	{
	}

	Model::Model(const String& path)
	{
        String filePath;
#ifdef LUMOS_PLATFORM_MOBILE
        if (filePath.find_last_of("/") != String::npos)
            filePath = filePath.substr(filePath.find_last_of("/") + 1);
#else
        filePath = path;
#endif
		LoadModel(filePath);
	}

	Model::Model(const Model& model) : m_CleanUpMesh(false)
	{
		for (const auto& mesh : model.GetMeshs())
		{
			m_Meshes.push_back(std::make_shared<Mesh>(*mesh.get()));
		}
	}

	Model::Model(const Mesh& mesh) :m_CleanUpMesh(false)
	{
		m_Meshes.push_back(std::make_shared<Mesh>(mesh));
	}

	Model::Model(const Mesh* mesh) : m_CleanUpMesh(false)
	{
		m_Meshes.push_back(std::make_shared<Mesh>(*mesh));
	}

	Model::Model(std::shared_ptr<Mesh> mesh) : m_CleanUpMesh(false)
	{
		m_Meshes.push_back(mesh);
	}

	Model::~Model()
	{
		m_Meshes.clear();
		m_Textures.clear();
	}

	void Model::LoadModel(const String& path)
	{
		std::string physicalPath;
		Lumos::VFS::Get()->ResolvePhysicalPath(path, physicalPath);

		String resolvedPath = physicalPath;

		m_Directory = resolvedPath.substr(0, resolvedPath.find_last_of('/'));

		const String fileExtension = StringFormat::GetFilePathExtension(path);

		if (fileExtension == "obj")
			LoadOBJ(resolvedPath);
		else if (fileExtension == "gltf" || fileExtension == "glb")
			LoadGLTF(resolvedPath);
		else
			LUMOS_CORE_ERROR("Unsupported File Type : {0}", fileExtension);

		LUMOS_CORE_INFO("Loaded Model - {0}", resolvedPath);
	}

	void Model::Draw(const bool bind)
	{
		for (auto & mesh : m_Meshes)
		{
			if (mesh)
			{
				mesh->Draw(bind);
			}
		}
	}

	void Model::SetMaterial(std::shared_ptr<Material>& material)
	{
		if(material)
		{
			for (auto& mesh : m_Meshes)
			{
				if (mesh)
					mesh->SetMaterial(material);
			}
		}
	}

	void Model::SetMaterialFlag(Material::RenderFlags flag)
	{
		for (auto& mesh : m_Meshes)
		{
			if (mesh && mesh->GetMaterial())
				mesh->GetMaterial()->SetRenderFlag(flag);
		}
	}

	void Model::ResetRenderFlags()
	{
		for (auto& mesh : m_Meshes)
		{
			if (mesh)
				mesh->GetMaterial()->SetRenderFlags(0);
		}
	}
}
