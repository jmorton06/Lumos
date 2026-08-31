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
#include "AssetRegistry.h"
#include <unordered_map>
#include <string>

namespace Lumos
{
    class StringPool;

    namespace Graphics
    {
        class Model;
        class Material;
    }

    class AssetManager
    {
    public:
        AssetManager();
        ~AssetManager();

        NONCOPYABLEANDMOVE(AssetManager);

        SharedPtr<Asset> GetAsset(UUID ID);
        AssetMetaData& AddAsset(const String8& name, SharedPtr<Asset> data, bool keepUnreferenced = true, bool isEngineAsset = false);
        AssetMetaData& AddAsset(UUID name, SharedPtr<Asset> data, bool keepUnreferenced = true, bool isEngineAsset = false);
        AssetMetaData GetAsset(const String8& name);

        SharedPtr<Asset> GetAssetData(const String8& name);

        void Destroy();
        void Update(float elapsedSeconds);
        bool AssetExists(const String8& name);
        bool AssetExists(UUID name);
        bool LoadShader(const String8& filePath, SharedPtr<Graphics::Shader>& shader, bool keepUnreferenced = true);
        bool LoadAsset(const String8& filePath, SharedPtr<Graphics::Shader>& shader, bool keepUnreferenced = true);

        SharedPtr<Asset> operator[](UUID name) { return GetAsset(name); }
        SharedPtr<Graphics::Texture2D> LoadTextureAsset(const String8& filePath, bool thread);
        SharedPtr<Graphics::Texture2D> LoadTextureAsset(const String8& filePath, bool thread, const Graphics::TextureDesc& desc);

        SharedPtr<Graphics::Material> LoadMaterialAsset(const String8& lmatPath);

        void RemoveAsset(const String8& name)
        {
            UUID id;
            if(m_AssetRegistry->GetID(name, id))
                m_AssetRegistry->Remove(id);
        }

        SharedPtr<AssetRegistry> GetAssetRegistry() { return m_AssetRegistry; }

        void SetTextureHotReloadEnabled(bool enabled) { m_HotReloadEnabled = enabled; }
        bool IsTextureHotReloadEnabled() const { return m_HotReloadEnabled; }

    protected:
        bool LoadTexture(const String8& filePath, SharedPtr<Graphics::Texture2D>& texture, bool thread);
        bool LoadTexture(const String8& filePath, SharedPtr<Graphics::Texture2D>& texture, bool thread, const Graphics::TextureDesc& desc);

        struct TextureWatch
        {
            std::string Path;                            // physical filesystem path
            WeakPtr<Graphics::Texture2D> Texture;        // weak so unloaded textures drop out
            Graphics::TextureDesc Desc;
            uint64_t LastModTime = 0;
        };
        void RegisterTextureWatch(const String8& physicalPath, const SharedPtr<Graphics::Texture2D>& texture, const Graphics::TextureDesc& desc);
        void TickHotReload(float elapsedSeconds);

        Arena* m_Arena;
        SharedPtr<AssetRegistry> m_AssetRegistry;
        SharedPtr<StringPool> m_StringPool;

        // Separate material cache (Material doesn't inherit Asset)
        std::unordered_map<std::string, SharedPtr<Graphics::Material>> m_MaterialCache;

        std::vector<TextureWatch> m_TextureWatches;
        float m_HotReloadAccum   = 0.0f;
        bool m_HotReloadEnabled  = false;
    };
}
