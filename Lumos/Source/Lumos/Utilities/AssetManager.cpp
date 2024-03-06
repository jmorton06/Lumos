#include "Precompiled.h"
#include "AssetManager.h"
#include "Core/Application.h"
#include <future>

namespace Lumos
{
    inline static std::vector<std::future<void>> m_Futures;

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

        LUMOS_ASSERT(m_AssetRegistry.find(handle) != m_AssetRegistry.end());
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
