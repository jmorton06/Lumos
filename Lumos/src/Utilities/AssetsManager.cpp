#include "LM.h"
#include "AssetsManager.h"
#include "Graphics/Mesh.h"
#include "Graphics/MeshFactory.h"
#include "Graphics/ModelLoader/ModelLoader.h"
#include "Entity/Entity.h"
#include "Entity/Component/Components.h"

namespace lumos
{
	AssetManager<graphics::Mesh>*	 AssetsManager::s_DefaultModels   = nullptr;
	AssetManager<graphics::Texture2D>* AssetsManager::s_DefaultTextures = nullptr;

	void AssetsManager::InitializeMeshes()
	{
		s_DefaultModels   = new AssetManager<graphics::Mesh>();
		s_DefaultTextures = new AssetManager<graphics::Texture2D>();

        s_DefaultModels->AddAsset("Cube", std::shared_ptr<graphics::Mesh>(graphics::CreateCube(2.0f,nullptr)));
        s_DefaultModels->AddAsset("Pyramid", std::shared_ptr<graphics::Mesh>(graphics::CreatePyramid(1.0f,nullptr)));
        s_DefaultModels->AddAsset("Sphere", std::shared_ptr<graphics::Mesh>(graphics::CreateSphere(64,64, nullptr)));
	}

	void AssetsManager::ReleaseMeshes()
	{
		delete s_DefaultModels;
		delete s_DefaultTextures;
	}
}
