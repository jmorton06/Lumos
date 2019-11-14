#include "lmpch.h"
#include "AssetsManager.h"
#include "Graphics/Mesh.h"
#include "Graphics/MeshFactory.h"
#include "Graphics/ModelLoader/ModelLoader.h"
#include "ECS/Component/Components.h"

namespace Lumos
{
	AssetManager<Graphics::Mesh>* AssetsManager::s_DefaultModels = nullptr;
	AssetManager<Graphics::Texture2D>* AssetsManager::s_DefaultTextures = nullptr;

	void AssetsManager::InitializeMeshes()
	{
		s_DefaultModels   = lmnew AssetManager<Graphics::Mesh>();
		s_DefaultTextures = lmnew AssetManager<Graphics::Texture2D>();

        s_DefaultModels->Add("Cube", Ref<Graphics::Mesh>(Graphics::CreatePrimative(Graphics::PrimitiveType::Cube)));
        s_DefaultModels->Add("Pyramid", Ref<Graphics::Mesh>(Graphics::CreatePrimative(Graphics::PrimitiveType::Pyramid)));
		s_DefaultModels->Add("Sphere", Ref<Graphics::Mesh>(Graphics::CreatePrimative(Graphics::PrimitiveType::Sphere)));
	}

	void AssetsManager::ReleaseMeshes()
	{
		lmdel s_DefaultModels;
		lmdel s_DefaultTextures;
	}
}
