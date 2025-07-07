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

namespace Lumos
{
	class StringPool;
	
    namespace Graphics
    {
        class Model;
    }

    class AssetManager
    {
    public:
        AssetManager();
        ~AssetManager();

        NONCOPYABLEANDMOVE(AssetManager);

        SharedPtr<Asset> GetAsset(UUID ID);
        AssetMetaData& AddAsset(const String8& name, SharedPtr<Asset> data, bool keepUnreferenced = true);
        AssetMetaData& AddAsset(UUID name, SharedPtr<Asset> data, bool keepUnreferenced = true);
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

		SharedPtr<AssetRegistry> GetAssetRegistry() { return m_AssetRegistry; }

    protected:
        bool LoadTexture(const String8& filePath, SharedPtr<Graphics::Texture2D>& texture, bool thread);

        Arena* m_Arena;
        SharedPtr<AssetRegistry> m_AssetRegistry;
		SharedPtr<StringPool> m_StringPool;
    };
}
