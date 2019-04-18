#include "LM.h"
#include "AssetsManager.h"
#include "Graphics/Mesh.h"
#include "Graphics/MeshFactory.h"

namespace Lumos
{
	AssetManager<Mesh>*	 AssetsManager::s_DefaultModels   = nullptr;
	AssetManager<Texture2D>* AssetsManager::s_DefaultTextures = nullptr;

	void AssetsManager::InitializeMeshes()
	{
		s_DefaultModels   = new AssetManager<Mesh>();
		s_DefaultTextures = new AssetManager<Texture2D>();

        s_DefaultModels->AddAsset("Cube", std::shared_ptr<Mesh>(MeshFactory::CreateCube(2.0f,nullptr)));
		s_DefaultModels->AddAsset("Sphere", std::shared_ptr<Mesh>(MeshFactory::CreateCube(2.0f, nullptr)));// "/CoreMeshes/sphere.obj"));
		s_DefaultModels->AddAsset("Pyramid", std::shared_ptr<Mesh>(MeshFactory::CreateCube(2.0f, nullptr)));//"/CoreMeshes/pyramid.obj"));

	}

	void AssetsManager::ReleaseMeshes()
	{
		delete s_DefaultModels;
		delete s_DefaultTextures;
	}
}
