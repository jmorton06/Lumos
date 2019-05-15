#include "LM.h"
#include "AssetsManager.h"
#include "Graphics/Mesh.h"
#include "Graphics/MeshFactory.h"
#include "Graphics/ModelLoader/ModelLoader.h"
#include "Entity/Entity.h"
#include "Entity/Component/Components.h"

namespace Lumos
{
	AssetManager<Mesh>*	 AssetsManager::s_DefaultModels   = nullptr;
	AssetManager<Texture2D>* AssetsManager::s_DefaultTextures = nullptr;

	void AssetsManager::InitializeMeshes()
	{
		s_DefaultModels   = new AssetManager<Mesh>();
		s_DefaultTextures = new AssetManager<Texture2D>();

        s_DefaultModels->AddAsset("Cube", std::shared_ptr<Mesh>(MeshFactory::CreateCube(2.0f,nullptr)));
        s_DefaultModels->AddAsset("Pyramid", std::shared_ptr<Mesh>(MeshFactory::CreatePyramid(1.0f,nullptr)));
        s_DefaultModels->AddAsset("Sphere", std::shared_ptr<Mesh>(MeshFactory::CreateSphere(64,64, nullptr)));
	}

	void AssetsManager::ReleaseMeshes()
	{
		delete s_DefaultModels;
		delete s_DefaultTextures;
	}
}
