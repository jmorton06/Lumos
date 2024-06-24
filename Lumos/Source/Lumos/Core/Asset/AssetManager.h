#pragma once
#include "Core/Engine.h"
#include "Audio/Sound.h"
#include "Graphics/RHI/Shader.h"
#include "Utilities/TSingleton.h"
#include "Utilities/LoadImage.h"
#include "Graphics/Font.h"
#include "Graphics/Model.h"
#include "Utilities/CombineHash.h"
#include "Asset.h"
#include "AssetMetaData.h"

namespace Lumos
{
    class AssetRegistry;
    namespace Graphics
    {
        class Model;
    }

    class AssetManager
    {
    public:
        AssetManager();
        ~AssetManager();

        SharedPtr<Asset> GetAsset(UUID ID);
        AssetMetaData& AddAsset(const std::string& name, SharedPtr<Asset> data, bool keepUnreferenced = false);
        AssetMetaData& AddAsset(UUID name, SharedPtr<Asset> data, bool keepUnreferenced = false);
        AssetMetaData GetAsset(const std::string& name);

        SharedPtr<Asset> GetAssetData(const std::string& name);

        void Destroy();
        void Update(float elapsedSeconds);
        bool AssetExists(const std::string& name);
        bool AssetExists(UUID name);
        bool LoadShader(const std::string& filePath, SharedPtr<Graphics::Shader>& shader, bool keepUnreferenced = true);
        bool LoadAsset(const std::string& filePath, SharedPtr<Graphics::Shader>& shader, bool keepUnreferenced = true);

        SharedPtr<Asset> operator[](UUID name) { return GetAsset(name); }
        SharedPtr<Graphics::Texture2D> LoadTextureAsset(const std::string& filePath, bool thread);

        template <typename TAsset, typename... TArgs>
        SharedPtr<Asset> CreateMemoryOnlyAsset(const char* name, TArgs&&... args)
        {
            static_assert(std::is_base_of<Asset, TAsset>::value, "CreateMemoryOnlyAsset only works for types derived from Asset");

            uint64_t hash = 0;
            HashCombine(hash, std::forward<TArgs>(args)...);
            SharedPtr<TAsset> asset = SharedPtr<TAsset>::CreateSharedPtr(std::forward<TArgs>(args)...);
            AssetMetaData metaData  = AddAsset(name, asset);
            metaData.IsMemoryAsset  = true;
            metaData.ParameterCache = hash;
            return asset;
        }

        template <typename TAsset>
        SharedPtr<Asset> CreateMemoryOnlyAsset(const char* name, TAsset* asset)
        {
            static_assert(std::is_base_of<Asset, TAsset>::value, "CreateMemoryOnlyAsset only works for types derived from Asset");

            uint64_t hash                 = 0;
            SharedPtr<TAsset> sharedAsset = SharedPtr<TAsset>::SharedPtr(asset);
            AssetMetaData metaData        = AddAsset(name, sharedAsset);
            metaData.IsMemoryAsset        = true;
            return asset;
        }

        AssetRegistry* GetAssetRegistry() { return m_AssetRegistry; }

    protected:
        bool LoadTexture(const std::string& filePath, SharedPtr<Graphics::Texture2D>& texture, bool thread);

        Arena* m_Arena;
        AssetRegistry* m_AssetRegistry;
    };
}
