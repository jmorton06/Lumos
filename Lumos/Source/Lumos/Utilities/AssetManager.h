#pragma once
#include "Core/Engine.h"
#include "Audio/Sound.h"
#include "Graphics/RHI/Shader.h"
#include "Utilities/TSingleton.h"
#include "Utilities/LoadImage.h"
#include "Graphics/Font.h"
#include "Graphics/Model.h"
#include "Core/Asset.h"
#include "Utilities/CombineHash.h"

namespace Lumos
{
    namespace Graphics
    {
        class Model;
    }

    struct AssetMetaData
    {
        float timeSinceReload = 0.0f;
        float lastAccessed    = 0.0f;
        SharedPtr<Asset> data = nullptr;
        bool onDisk           = false;
        bool Expire           = true;
        AssetType Type = AssetType::Unkown;
        bool IsDataLoaded  = false;
        bool IsMemoryAsset = false;
        uint64_t ParameterCache;
    };

    class AssetRegistry
    {
        template <typename Archive>
        friend void save(Archive& archive, const AssetRegistry& registry);

        template <typename Archive>
        friend void load(Archive& archive, AssetRegistry& registry);

    public:
        AssetMetaData& operator[](const UUID handle);
        AssetMetaData& Get(const UUID handle);
        const AssetMetaData& Get(const UUID handle) const;

        void Update(float elapsedSeconds)
        {
            static UUID keysToDelete[256];
            std::size_t keysToDeleteCount = 0;

            for(auto&& [key, value] : m_AssetRegistry)
            {
                if(value.Expire && value.IsDataLoaded && value.data.GetCounter()->GetReferenceCount() == 1 && m_ExpirationTime < (elapsedSeconds - value.lastAccessed))
                {
                    keysToDelete[keysToDeleteCount] = key;
                    keysToDeleteCount++;
                }
            }

            for(std::size_t i = 0; i < keysToDeleteCount; i++)
            {
                m_AssetRegistry.erase(keysToDelete[i]);
            }
        }
        size_t Count() const { return m_AssetRegistry.size(); }
        bool Contains(const UUID handle) const;
        size_t Remove(const UUID handle);
        void Clear();

        auto begin() { return m_AssetRegistry.begin(); }
        auto end() { return m_AssetRegistry.end(); }
        auto begin() const { return m_AssetRegistry.cbegin(); }
        auto end() const { return m_AssetRegistry.cend(); }

        void AddName(const std::string& name, UUID ID)
        {
            m_NameMap.emplace(name, ID);
#ifndef LUMOS_PRODUCTION
            m_UUIDNameMap.emplace(ID, name);
#endif
        }
        bool GetID(const std::string& name, UUID& ID)
        {
            if(m_NameMap.find(name) != m_NameMap.end())
            {
                ID = m_NameMap[name];
                return true;
            }

            return false;
        }

#ifndef LUMOS_PRODUCTION
        bool GetName(UUID ID, std::string& name)
        {
            if(m_UUIDNameMap.find(ID) != m_UUIDNameMap.end())
            {
                name = m_UUIDNameMap[ID];
                return true;
            }

            return false;
        }
#endif

    private:
        std::unordered_map<UUID, AssetMetaData> m_AssetRegistry;
        std::unordered_map<std::string, UUID> m_NameMap;
#ifndef LUMOS_PRODUCTION
        std::unordered_map<UUID, std::string> m_UUIDNameMap; // Debug Only
#endif

        float m_ExpirationTime = 3.0f;
    };

    class AssetManager
    {
    public:
        AssetManager()
        {
            m_Arena = ArenaAlloc(Megabytes(4));
        }

        ~AssetManager()
        {
            ArenaRelease(m_Arena);
        }

        SharedPtr<Asset> GetAsset(UUID ID)
        {
            if(m_AssetRegistry.Contains(ID))
            {
                AssetMetaData& metaData = m_AssetRegistry[ID];
                metaData.lastAccessed   = (float)Engine::GetTimeStep().GetElapsedSeconds();
                return metaData.data;
            }

            LUMOS_LOG_WARN("Asset not found {0}", ID);
            return nullptr;
        }

        AssetMetaData& AddAsset(const std::string& name, SharedPtr<Asset> data, bool keepUnreferenced = false)
        {
            UUID ID = UUID();
            m_AssetRegistry.GetID(name, ID);

            AssetMetaData& metaData = AddAsset(ID, data, keepUnreferenced);
            m_AssetRegistry.AddName(name, ID);
            return metaData;
        }

        AssetMetaData& AddAsset(UUID name, SharedPtr<Asset> data, bool keepUnreferenced = false)
        {
            if(m_AssetRegistry.Contains(name))
            {
                AssetMetaData& metaData = m_AssetRegistry[name];
                metaData.lastAccessed   = (float)Engine::GetTimeStep().GetElapsedSeconds();
                metaData.data           = data;
                metaData.Expire         = !keepUnreferenced;
                metaData.Type           = data ?  data->GetAssetType() : AssetType::Unkown;
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

            m_AssetRegistry[name] = newResource;

            return m_AssetRegistry[name];
        }

        AssetMetaData GetAsset(const std::string& name)
        {
            UUID ID;
            if(m_AssetRegistry.GetID(name, ID))
            {
                if (m_AssetRegistry[ID].IsDataLoaded)
                    return m_AssetRegistry[ID];
                else
                {
                    return AssetMetaData();
                }
            }

            return AssetMetaData();
        }

        SharedPtr<Asset> GetAssetData(const std::string& name)
        {
            return GetAsset(name).data;
        }

        virtual void Destroy()
        {
            m_AssetRegistry.Clear();
        }

        void Update(float elapsedSeconds)
        {
            m_AssetRegistry.Update(elapsedSeconds);
        }

        bool AssetExists(const std::string& name)
        {
            UUID ID;
            if(m_AssetRegistry.GetID(name, ID))
                return m_AssetRegistry.Contains(ID);

            return false;
        }

        bool AssetExists(UUID name)
        {
            return m_AssetRegistry.Contains(name);
        }

        SharedPtr<Asset> operator[](UUID name)
        {
            return GetAsset(name);
        }

        bool LoadShader(const std::string& filePath, SharedPtr<Graphics::Shader>& shader, bool keepUnreferenced = true)
        {
            shader = SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromFile(filePath));
            AddAsset(filePath, shader, keepUnreferenced);
            return true;
        }

        bool LoadAsset(const std::string& filePath, SharedPtr<Graphics::Shader>& shader, bool keepUnreferenced = true)
        {
            shader = SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromFile(filePath));
            AddAsset(filePath, shader, keepUnreferenced);
            return true;
        }

        SharedPtr<Graphics::Texture2D> LoadTextureAsset(const std::string& filePath, bool thread);

        template <typename TAsset, typename... TArgs>
        SharedPtr<Asset> CreateMemoryOnlyAsset(const char* name, TArgs&&... args)
        {
            static_assert(std::is_base_of<Asset, TAsset>::value, "CreateMemoryOnlyAsset only works for types derived from Asset");

            uint64_t hash = 0;
            HashCombine(hash, std::forward<TArgs>(args)...);
            SharedPtr<TAsset> asset = SharedPtr<TAsset>::CreateSharedPtr(std::forward<TArgs>(args)...);
            AssetMetaData metaData = AddAsset(name, asset);
            metaData.IsMemoryAsset = true;
            metaData.ParameterCache = hash;
            return asset;
        }

        template <typename TAsset>
        SharedPtr<Asset> CreateMemoryOnlyAsset(const char* name, TAsset* asset)
        {
            static_assert(std::is_base_of<Asset, TAsset>::value, "CreateMemoryOnlyAsset only works for types derived from Asset");

            uint64_t hash = 0;
            SharedPtr<TAsset> sharedAsset = SharedPtr<TAsset>::SharedPtr(asset);
            AssetMetaData metaData = AddAsset(name, sharedAsset);
            metaData.IsMemoryAsset = true;
            return asset;
        }

        AssetRegistry& GetAssetRegistry() { return m_AssetRegistry; }

    protected:
        bool LoadTexture(const std::string& filePath, SharedPtr<Graphics::Texture2D>& texture, bool thread);

        Arena* m_Arena;
        AssetRegistry m_AssetRegistry;
    };
}
