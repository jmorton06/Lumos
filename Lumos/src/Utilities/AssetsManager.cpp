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
        
        auto pEntity = ModelLoader::LoadModel("/CoreMeshes/pyramid.obj");
        auto sEntity = ModelLoader::LoadModel("/CoreMeshes/sphere.obj");
        
        if(!pEntity->GetComponent<MeshComponent>())
        {
            for(auto child : pEntity->GetChildren())
            {
                if(child->GetComponent<MeshComponent>())
                {
                    s_DefaultModels->AddAsset("Pyramid", child->GetComponent<MeshComponent>()->m_Model);
                    break;
                }
            }
        }
        else
        {
            s_DefaultModels->AddAsset("Pyramid", pEntity->GetComponent<MeshComponent>()->m_Model);
        }
        
        if(!sEntity->GetComponent<MeshComponent>())
        {
            for(auto child : sEntity->GetChildren())
            {
                if(child->GetComponent<MeshComponent>())
                {
                    s_DefaultModels->AddAsset("Sphere", child->GetComponent<MeshComponent>()->m_Model);
                    break;
                }
            }
        }
        else
        {
            s_DefaultModels->AddAsset("Sphere", sEntity->GetComponent<MeshComponent>()->m_Model);
        }
	}

	void AssetsManager::ReleaseMeshes()
	{
		delete s_DefaultModels;
		delete s_DefaultTextures;
	}
}
