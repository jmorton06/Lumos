#include "LM.h"
#include "AssetsManager.h"
#include "Graphics/Model/Model.h"
#include "Graphics/MeshFactory.h"

namespace Lumos
{
	AssetManager<Model>*	 AssetsManager::s_DefaultModels   = nullptr;
	AssetManager<Texture2D>* AssetsManager::s_DefaultTextures = nullptr;

	void AssetsManager::InitializeMeshes()
	{
		s_DefaultModels   = new AssetManager<Model>();
		s_DefaultTextures = new AssetManager<Texture2D>();

        s_DefaultModels->AddAsset("Cube", std::make_shared<Model>(std::shared_ptr<Mesh>(MeshFactory::CreateCube(2.0f,nullptr))));
		s_DefaultModels->AddAsset("Sphere", std::make_shared<Model>("/CoreMeshes/sphere.obj"));
		s_DefaultModels->AddAsset("Pyramid", std::make_shared<Model>("/CoreMeshes/pyramid.obj"));

	}

	void AssetsManager::ReleaseMeshes()
	{
		delete s_DefaultModels;
		delete s_DefaultTextures;
	}
}
