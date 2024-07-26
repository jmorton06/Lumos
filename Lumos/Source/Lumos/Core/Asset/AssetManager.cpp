#include "Precompiled.h"
#include "AssetManager.h"
#include "AssetRegistry.h"
#include "Core/Application.h"
#include "Graphics/RHI/Texture.h"
#include <future>

namespace Lumos
{
    static std::vector<std::future<void>> m_Futures;

    AssetManager::AssetManager()
    {
        m_Arena         = ArenaAlloc(Megabytes(4));
        m_AssetRegistry = new AssetRegistry();
    }

    AssetManager::~AssetManager()
    {
        ArenaRelease(m_Arena);
        delete m_AssetRegistry;
    }

    SharedPtr<Asset> AssetManager::GetAsset(UUID ID)
    {
        if(m_AssetRegistry->Contains(ID))
        {
            AssetMetaData& metaData = (*m_AssetRegistry)[ID];
            metaData.lastAccessed   = (float)Engine::GetTimeStep().GetElapsedSeconds();
            return metaData.data;
        }

        LWARN("Asset not found {0}", (u64)ID);
        return nullptr;
    }

    AssetMetaData& AssetManager::AddAsset(const std::string& name, SharedPtr<Asset> data, bool keepUnreferenced)
    {
        UUID ID = UUID();
        m_AssetRegistry->GetID(name, ID);

        AssetMetaData& metaData = AddAsset(ID, data, keepUnreferenced);
        m_AssetRegistry->AddName(name, ID);
        return metaData;
    }

    AssetMetaData& AssetManager::AddAsset(UUID name, SharedPtr<Asset> data, bool keepUnreferenced)
    {
        AssetRegistry& registry = *m_AssetRegistry;
        if(m_AssetRegistry->Contains(name))
        {
            AssetMetaData& metaData = registry[name];
            metaData.lastAccessed   = (float)Engine::GetTimeStep().GetElapsedSeconds();
            metaData.data           = data;
            metaData.Expire         = !keepUnreferenced;
            metaData.Type           = data ? data->GetAssetType() : AssetType::Unkown;
            metaData.IsDataLoaded   = data ? true : false;
            return metaData;
        }

        AssetMetaData newResource;
        newResource.data            = data;
        newResource.timeSinceReload = 0;
        newResource.onDisk          = true;
        newResource.lastAccessed    = (float)Engine::GetTimeStep().GetElapsedSeconds();
        newResource.Type            = data->GetAssetType();
        newResource.Expire          = !keepUnreferenced;
        newResource.IsDataLoaded    = data ? true : false;

        registry[name] = newResource;

        return registry[name];
    }

    AssetMetaData AssetManager::GetAsset(const std::string& name)
    {
        UUID ID;
        if(m_AssetRegistry->GetID(name, ID))
        {
            AssetRegistry& registry = *m_AssetRegistry;
            if(registry[ID].IsDataLoaded)
                return registry[ID];
            else
            {
                return AssetMetaData();
            }
        }

        return AssetMetaData();
    }

    SharedPtr<Asset> AssetManager::GetAssetData(const std::string& name)
    {
        return GetAsset(name).data;
    }

    void AssetManager::Destroy()
    {
        m_AssetRegistry->Clear();
    }

    void AssetManager::Update(float elapsedSeconds)
    {
        m_AssetRegistry->Update(elapsedSeconds);
    }

    bool AssetManager::AssetExists(const std::string& name)
    {
        UUID ID;
        if(m_AssetRegistry->GetID(name, ID))
            return m_AssetRegistry->Contains(ID) && (*m_AssetRegistry)[ID].IsDataLoaded;

        return false;
    }

    bool AssetManager::AssetExists(UUID name)
    {
        return m_AssetRegistry->Contains(name);
    }

    bool AssetManager::LoadShader(const std::string& filePath, SharedPtr<Graphics::Shader>& shader, bool keepUnreferenced)
    {
        shader = SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromFile(filePath));
        AddAsset(filePath, shader, keepUnreferenced);
        return true;
    }

    bool AssetManager::LoadAsset(const std::string& filePath, SharedPtr<Graphics::Shader>& shader, bool keepUnreferenced)
    {
        shader = SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromFile(filePath));
        AddAsset(filePath, shader, keepUnreferenced);
        return true;
    }

    static void LoadTexture2D(Graphics::Texture2D* tex, const std::string& path)
    {
        LUMOS_PROFILE_FUNCTION();
        ImageLoadDesc imageLoadDesc = {};
        imageLoadDesc.filePath      = path.c_str();
        imageLoadDesc.maxHeight     = 256;
        imageLoadDesc.maxWidth      = 256;
        Lumos::LoadImageFromFile(imageLoadDesc);

        Graphics::TextureDesc desc;
        desc.format = imageLoadDesc.outBits / 4 == 8 ? Graphics::RHIFormat::R8G8B8A8_Unorm : Graphics::RHIFormat::R32G32B32A32_Float;

        Application::Get().SubmitToMainThread([tex, imageLoadDesc, desc]()
                                              { tex->Load(imageLoadDesc.outWidth, imageLoadDesc.outHeight, imageLoadDesc.outPixels, desc); });
    }

    bool AssetManager::LoadTexture(const std::string& filePath, SharedPtr<Graphics::Texture2D>& texture, bool thread)
    {
        texture = SharedPtr<Graphics::Texture2D>(Graphics::Texture2D::Create({}, 1, 1));
        if(thread)
            m_Futures.push_back(std::async(std::launch::async, &LoadTexture2D, texture.get(), filePath));
        else
            LoadTexture2D(texture.get(), filePath);

        AddAsset(filePath, texture);
        return true;
    }

    SharedPtr<Graphics::Texture2D> AssetManager::LoadTextureAsset(const std::string& filePath, bool thread)
    {
        SharedPtr<Graphics::Texture2D> texture;
        LoadTexture(filePath, texture, thread);
        return texture;
    }

    static std::mutex s_AssetRegistryMutex;

    AssetMetaData& AssetRegistry::operator[](UUID handle)
    {
        std::scoped_lock<std::mutex> lock(s_AssetRegistryMutex);
        return m_AssetRegistry[handle];
    }

    const AssetMetaData& AssetRegistry::Get(UUID handle) const
    {
        std::scoped_lock<std::mutex> lock(s_AssetRegistryMutex);

        ASSERT(m_AssetRegistry.find(handle) != m_AssetRegistry.end());
        return m_AssetRegistry.at(handle);
    }

    AssetMetaData& AssetRegistry::Get(UUID handle)
    {
        std::scoped_lock<std::mutex> lock(s_AssetRegistryMutex);
        return m_AssetRegistry[handle];
    }

    bool AssetRegistry::Contains(UUID handle) const
    {
        std::scoped_lock<std::mutex> lock(s_AssetRegistryMutex);
        return m_AssetRegistry.find(handle) != m_AssetRegistry.end();
    }

    size_t AssetRegistry::Remove(UUID handle)
    {
        std::scoped_lock<std::mutex> lock(s_AssetRegistryMutex);
        return m_AssetRegistry.erase(handle);
    }

    void AssetRegistry::Clear()
    {
        std::scoped_lock<std::mutex> lock(s_AssetRegistryMutex);
        m_AssetRegistry.clear();
    }
}
