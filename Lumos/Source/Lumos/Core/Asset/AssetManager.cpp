#include "Precompiled.h"
#include "AssetManager.h"
#include "AssetRegistry.h"
#include "Core/Application.h"
#include "Core/OS/FileSystem.h"
#include "Graphics/RHI/Texture.h"
#include "Graphics/RHI/Shader.h"
#include "Graphics/Material.h"
#include "Utilities/StringPool.h"
#include "Maths/MathsSerialisation.h"
#include <cereal/archives/json.hpp>
#include <cereal/cereal.hpp>
#include <future>
#include <inttypes.h>
#include <sstream>

namespace Lumos
{
    static TDArray<std::future<void>> m_Futures;

    AssetManager::AssetManager()
    {
        m_Arena         = ArenaAlloc(Megabytes(4));
        m_StringPool    = CreateSharedPtr<StringPool>(m_Arena, 260);
        m_AssetRegistry = CreateSharedPtr<AssetRegistry>();
    }

    AssetManager::~AssetManager()
    {
        ArenaRelease(m_Arena);
    }

    SharedPtr<Asset> AssetManager::GetAsset(UUID ID)
    {
        if(m_AssetRegistry->Contains(ID))
        {
            AssetMetaData& metaData = (*m_AssetRegistry)[ID];
            metaData.LastAccessed   = (float)Engine::GetTimeStep().GetElapsedSeconds();
            return metaData.Data;
        }

        LWARN("Asset not found %" PRIu64, (u64)ID);
        return nullptr;
    }

    AssetMetaData& AssetManager::AddAsset(const String8& name, SharedPtr<Asset> data, bool keepUnreferenced, bool isEngineAsset)
    {
        UUID ID = UUID();
        m_AssetRegistry->GetID(name, ID);

        AssetMetaData& metaData = AddAsset(ID, data, keepUnreferenced, isEngineAsset);
        m_AssetRegistry->AddName(name, ID);
        return metaData;
    }

    AssetMetaData& AssetManager::AddAsset(UUID ID, SharedPtr<Asset> data, bool keepUnreferenced, bool isEngineAsset)
    {
        AssetRegistry& registry = *m_AssetRegistry;
        AssetMetaData* existing = registry.GetPtr(ID);
        if(existing)
        {
            existing->LastAccessed  = (float)Engine::GetTimeStep().GetElapsedSeconds();
            existing->Data          = data;
            existing->Expire        = !keepUnreferenced;
            existing->Type          = data ? data->GetAssetType() : AssetType::Unkown;
            existing->IsDataLoaded  = data ? true : false;
            existing->IsEngineAsset = isEngineAsset || existing->IsEngineAsset;
            return *existing;
        }

        AssetMetaData newResource;
        newResource.Data            = data;
        newResource.TimeSinceReload = 0;
        newResource.bEmbeddedAsset  = false;
        newResource.LastAccessed    = (float)Engine::GetTimeStep().GetElapsedSeconds();
        newResource.Type            = data ? data->GetAssetType() : AssetType::Unkown;
        newResource.Expire          = !keepUnreferenced;
        newResource.IsDataLoaded    = data ? true : false;
        newResource.IsEngineAsset   = isEngineAsset;

        registry.Insert(ID, newResource);

        return registry.Get(ID);
    }

    AssetMetaData AssetManager::GetAsset(const String8& name)
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

    SharedPtr<Asset> AssetManager::GetAssetData(const String8& name)
    {
        return GetAsset(name).Data;
    }

    void AssetManager::Destroy()
    {
        m_AssetRegistry->Clear();
    }

    void AssetManager::Update(float elapsedSeconds)
    {
        m_AssetRegistry->Update(elapsedSeconds);
    }

    bool AssetManager::AssetExists(const String8& name)
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

    bool AssetManager::LoadShader(const String8& filePath, SharedPtr<Graphics::Shader>& shader, bool keepUnreferenced)
    {
        shader = SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromFile((const char*)filePath.str));
        AddAsset(filePath, shader, keepUnreferenced);
        return true;
    }

    bool AssetManager::LoadAsset(const String8& filePath, SharedPtr<Graphics::Shader>& shader, bool keepUnreferenced)
    {
        shader = SharedPtr<Graphics::Shader>(Graphics::Shader::CreateFromFile((const char*)filePath.str));
        AddAsset(filePath, shader, keepUnreferenced);
        return true;
    }

    static void LoadTexture2D(Graphics::Texture2D* tex, const String8& path, bool deferred, Graphics::TextureDesc desc = {})
    {
        LUMOS_PROFILE_FUNCTION();
        ImageLoadDesc imageLoadDesc = {};
        imageLoadDesc.filePath      = (const char*)path.str;
        imageLoadDesc.maxHeight     = 2048;
        imageLoadDesc.maxWidth      = 2048;
        Lumos::LoadImageFromFile(imageLoadDesc);

        desc.format    = imageLoadDesc.outBits / 4 == 8 ? Graphics::RHIFormat::R8G8B8A8_Unorm : Graphics::RHIFormat::R32G32B32A32_Float;
        if(desc.minFilter == Graphics::TextureFilter::NONE)
            desc.minFilter = Graphics::TextureFilter::LINEAR;
        if(desc.magFilter == Graphics::TextureFilter::NONE)
            desc.magFilter = Graphics::TextureFilter::LINEAR;

        if(deferred)
        {
            Application::Get().SubmitToMainThread([tex, imageLoadDesc, desc]()
            {
                tex->Load(imageLoadDesc.outWidth, imageLoadDesc.outHeight, imageLoadDesc.outPixels, desc);
                delete[] static_cast<uint8_t*>(imageLoadDesc.outPixels);
            });
        }
        else
        {
            tex->Load(imageLoadDesc.outWidth, imageLoadDesc.outHeight, imageLoadDesc.outPixels, desc);
            delete[] static_cast<uint8_t*>(imageLoadDesc.outPixels);
        }
    }

    bool AssetManager::LoadTexture(const String8& filePath, SharedPtr<Graphics::Texture2D>& texture, bool thread)
    {
        texture = SharedPtr<Graphics::Texture2D>(Graphics::Texture2D::Create({}, 1, 1));
        if(thread)
            m_Futures.PushBack(std::async(std::launch::async, LoadTexture2D, texture.get(), filePath, true, Graphics::TextureDesc{}));
        else
            LoadTexture2D(texture.get(), filePath, false);

        AddAsset(filePath, texture);
        return true;
    }

    bool AssetManager::LoadTexture(const String8& filePath, SharedPtr<Graphics::Texture2D>& texture, bool thread, const Graphics::TextureDesc& desc)
    {
        texture = SharedPtr<Graphics::Texture2D>(Graphics::Texture2D::Create({}, 1, 1));
        if(thread)
            m_Futures.PushBack(std::async(std::launch::async, LoadTexture2D, texture.get(), filePath, true, desc));
        else
            LoadTexture2D(texture.get(), filePath, false, desc);

        AddAsset(filePath, texture);
        return true;
    }

    SharedPtr<Graphics::Texture2D> AssetManager::LoadTextureAsset(const String8& filePath, bool thread)
    {
        if(AssetExists(filePath))
        {
            auto existing = GetAssetData(filePath);
            if(existing)
                return existing.As<Graphics::Texture2D>();
        }

        SharedPtr<Graphics::Texture2D> texture;
        LoadTexture(filePath, texture, thread);
        return texture;
    }

    SharedPtr<Graphics::Texture2D> AssetManager::LoadTextureAsset(const String8& filePath, bool thread, const Graphics::TextureDesc& desc)
    {
        if(AssetExists(filePath))
        {
            auto existing = GetAssetData(filePath);
            if(existing)
                return existing.As<Graphics::Texture2D>();
        }

        SharedPtr<Graphics::Texture2D> texture;
        LoadTexture(filePath, texture, thread, desc);
        return texture;
    }

    SharedPtr<Graphics::Material> AssetManager::LoadMaterialAsset(const String8& lmatPath)
    {
        std::string key((const char*)lmatPath.str, lmatPath.size);
        auto it = m_MaterialCache.find(key);
        if(it != m_MaterialCache.end())
            return it->second;

        ArenaTemp scratch = ScratchBegin(0, 0);

        String8 physicalPath;
        if(!FileSystem::Get().ResolvePhysicalPath(scratch.arena, lmatPath, &physicalPath))
        {
            ScratchEnd(scratch);
            return nullptr;
        }

        if(!FileSystem::FileExists(physicalPath))
        {
            ScratchEnd(scratch);
            return nullptr;
        }

        int64_t fileSize = FileSystem::GetFileSize(physicalPath);
        if(fileSize <= 0)
        {
            ScratchEnd(scratch);
            return nullptr;
        }

        Arena* fileArena = ArenaAlloc((u64)fileSize + Kilobytes(1));
        String8 data = FileSystem::ReadTextFile(fileArena, physicalPath);
        if(data.size == 0)
        {
            ArenaRelease(fileArena);
            ScratchEnd(scratch);
            return nullptr;
        }

        // Parse .lmat JSON
        std::istringstream iss(std::string((const char*)data.str, data.size));

        static const char* texNames[] = { "Albedo", "Normal", "Metallic", "Roughness", "Ao", "Emissive" };

        std::string texPaths[6];
        Graphics::MaterialProperties props;
        std::string shaderName;

        try
        {
            cereal::JSONInputArchive ar(iss);
            for(u32 s = 0; s < 6; s++)
                ar(cereal::make_nvp(texNames[s], texPaths[s]));

            ar(cereal::make_nvp("albedoColour", props.albedoColour),
               cereal::make_nvp("roughnessValue", props.roughness),
               cereal::make_nvp("metallicValue", props.metallic),
               cereal::make_nvp("emissiveValue", props.emissive),
               cereal::make_nvp("albedoMapFactor", props.albedoMapFactor),
               cereal::make_nvp("metallicMapFactor", props.metallicMapFactor),
               cereal::make_nvp("roughnessMapFactor", props.roughnessMapFactor),
               cereal::make_nvp("normalMapFactor", props.normalMapFactor),
               cereal::make_nvp("aoMapFactor", props.occlusionMapFactor),
               cereal::make_nvp("emissiveMapFactor", props.emissiveMapFactor),
               cereal::make_nvp("alphaCutOff", props.alphaCutoff),
               cereal::make_nvp("workflow", props.workflow),
               cereal::make_nvp("shader", shaderName));

            ar(cereal::make_nvp("Reflectance", props.reflectance));
        }
        catch(...)
        {
            LERROR("AssetManager: Failed to parse .lmat %.*s", (int)lmatPath.size, lmatPath.str);
            ArenaRelease(fileArena);
            ScratchEnd(scratch);
            return nullptr;
        }

        ArenaRelease(fileArena);

        auto shaderAsset = GetAssetData(Str8Lit("ForwardPBR"));
        if(shaderAsset)
        {
            auto shaderPtr = shaderAsset.As<Graphics::Shader>();
            auto material  = CreateSharedPtr<Graphics::Material>(shaderPtr, props);

            Graphics::PBRMataterialTextures textures;
            for(u32 s = 0; s < 6; s++)
            {
                if(texPaths[s].empty())
                    continue;

                auto tex = LoadTextureAsset(Str8StdS(texPaths[s]), true);
                if(tex)
                {
                    switch(s)
                    {
                    case 0: textures.albedo    = tex; break;
                    case 1: textures.normal    = tex; break;
                    case 2: textures.metallic  = tex; break;
                    case 3: textures.roughness = tex; break;
                    case 4: textures.ao        = tex; break;
                    case 5: textures.emissive  = tex; break;
                    }
                }
            }
            material->SetTextures(textures);

            m_MaterialCache[key] = material;
            ScratchEnd(scratch);
            return material;
        }

        ScratchEnd(scratch);
        return nullptr;
    }
}
