#include "LM.h"
#include "AssetsManager.h"
#include "Graphics/Mesh.h"
#include "Graphics/MeshFactory.h"
#include "Graphics/ModelLoader/ModelLoader.h"
#include "ECS/EntityManager.h"
#include "ECS/Component/Components.h"

namespace Lumos
{
	AssetManager<Graphics::Mesh>* AssetsManager::s_DefaultModels   = nullptr;
	AssetManager<Graphics::Texture2D>* AssetsManager::s_DefaultTextures = nullptr;

	void AssetsManager::InitializeMeshes()
	{
		s_DefaultModels   = lmnew AssetManager<Graphics::Mesh>();
		s_DefaultTextures = lmnew AssetManager<Graphics::Texture2D>();

        auto cube = Ref<Graphics::Mesh>(Graphics::CreateCube(2.0f));
        s_DefaultModels->Add("Cube", cube);
        s_DefaultModels->Add("Pyramid", Ref<Graphics::Mesh>(Graphics::CreatePyramid(1.0f)));
		s_DefaultModels->Add("Sphere", Ref<Graphics::Mesh>(Graphics::CreateCapsule(1.0f,2.0f)));// Graphics::CreateSphere(64, 64)));
	}

	void AssetsManager::ReleaseMeshes()
	{
		delete s_DefaultModels;
		delete s_DefaultTextures;
	}
}
