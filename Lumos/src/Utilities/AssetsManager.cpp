#include "LM.h"
#include "AssetsManager.h"
#include "Graphics/Mesh.h"
#include "Graphics/MeshFactory.h"
#include "Graphics/ModelLoader/ModelLoader.h"
#include "ECS/Entity.h"
#include "ECS/Component/Components.h"

namespace Lumos
{
	AssetManager<Graphics::Mesh>*	 AssetsManager::s_DefaultModels   = nullptr;
	AssetManager<Graphics::Texture2D>* AssetsManager::s_DefaultTextures = nullptr;

	void AssetsManager::InitializeMeshes()
	{
		s_DefaultModels   = lmnew AssetManager<Graphics::Mesh>();
		s_DefaultTextures = lmnew AssetManager<Graphics::Texture2D>();

        s_DefaultModels->AddAsset("Cube", std::shared_ptr<Graphics::Mesh>(Graphics::CreateCube(2.0f)));
        s_DefaultModels->AddAsset("Pyramid", std::shared_ptr<Graphics::Mesh>(Graphics::CreatePyramid(1.0f)));
        s_DefaultModels->AddAsset("Sphere", std::shared_ptr<Graphics::Mesh>(Graphics::CreateSphere(64,64)));
	}

	void AssetsManager::ReleaseMeshes()
	{
		delete s_DefaultModels;
		delete s_DefaultTextures;
	}
}
