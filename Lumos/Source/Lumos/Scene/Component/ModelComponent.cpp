#include "Precompiled.h"
#include "ModelComponent.h"
#include "Core/Application.h"
#include "Core/Asset/AssetManager.h"
#include "Core/Asset/AssetImporter.h"
#include "Core/OS/FileSystem.h"
#include "Graphics/Mesh.h"
#include "Utilities/StringUtilities.h"
#include "Scene/SceneManager.h"
#include "Scene/Scene.h"
#include <entt/entity/registry.hpp>
#include <filesystem>

namespace Lumos::Graphics
{
    static bool IsSourceModelExtension(const std::string& ext)
    {
        return ext == "obj" || ext == "gltf" || ext == "glb" || ext == "fbx" || ext == "FBX";
    }

    ModelComponent::ModelComponent(const std::string& path)
    {
        LoadFromLibrary(path);
    }

    void ModelComponent::LoadFromLibrary(const std::string& path)
    {
        std::string loadPath = path;

        // Auto-import source model files to .lmesh
        // Note: don't switch loadPath to the imported path — Model finds it internally
        // via hash lookup, keeping GetFilePath() returning the source path for serialization.
        std::string ext = StringUtilities::GetFilePathExtension(path);
        if(Application::Get().GetProjectSettings().AutoImportMeshes && IsSourceModelExtension(ext))
        {
            if(AssetImporter::NeedsImport(path))
            {
                LINFO("Auto-importing source model: %s", path.c_str());
                ImportSettings settings;
                AssetImporter::Import(path, settings);
            }
        }
        else if(ext == "lmesh" && path.find("//Assets/Imported/") != std::string::npos)
        {
            // Migration: old scenes stored the imported .lmesh path directly.
            // Scan .meta files to find the original source path, then reimport.
            ArenaTemp scratch = ScratchBegin(nullptr, 0);
            const String8& assetsPathStr = FileSystem::Get().GetAssetPath();
            std::string assetsDir((const char*)assetsPathStr.str, assetsPathStr.size);
            std::string foundSource;

            try
            {
                for(auto& entry : std::filesystem::recursive_directory_iterator(assetsDir))
                {
                    if(!entry.is_regular_file())
                        continue;
                    std::string metaPath = entry.path().string();
                    if(metaPath.size() < 5 || metaPath.compare(metaPath.size() - 5, 5, ".meta") != 0)
                        continue;
                    std::string srcPhysical = metaPath.substr(0, metaPath.size() - 5);
                    std::string srcExt      = StringUtilities::GetFilePathExtension(srcPhysical);
                    if(!IsSourceModelExtension(srcExt))
                        continue;

                    // Build VFS path: strip assetsDir prefix, prepend //Assets/
                    std::string rel = srcPhysical.substr(assetsDir.size());
                    for(char& c : rel)
                        if(c == '\\') c = '/';
                    if(!rel.empty() && rel[0] == '/')
                        rel = rel.substr(1);
                    std::string vfsPath = "//Assets/" + rel;

                    if(AssetImporter::GetImportedPath(vfsPath) == path)
                    {
                        foundSource = vfsPath;
                        break;
                    }
                }
            }
            catch(...) {}

            ScratchEnd(scratch);

            if(!foundSource.empty())
            {
                LINFO("ModelComponent: migrating stale imported path -> %s", foundSource.c_str());
                ImportSettings settings;
                AssetImporter::Import(foundSource, settings);
                loadPath = foundSource;
            }
        }

        String8 Path = Str8StdS(loadPath);
        auto am      = Application::Get().GetAssetManager();
        if(am->AssetExists(Path))
        {
            ModelRef = am->GetAssetData(Path).As<Graphics::Model>();
            return;
        }
        ModelRef = am->AddAsset(Path, CreateSharedPtr<Graphics::Model>(loadPath)).Data.As<Graphics::Model>();
    }

    void ModelComponent::ApplyMaterialOverrides()
    {
        if(!ModelRef)
            return;
        auto& meshes = ModelRef->GetMeshes();
        for(auto& [idx, path] : MaterialOverrides)
        {
            if(idx < (u32)meshes.Size() && !path.empty())
                meshes[idx]->SetAndLoadMaterial(path);
        }
    }

    void ModelComponent::ApplyAnimationOverride()
    {
        if(!ModelRef || AnimationOverride.empty())
            return;
        ModelRef->LoadLAnim(AnimationOverride);
    }

    bool ModelComponent::ReloadSceneModels(const std::string& sourcePath)
    {
        auto scene = Application::Get().GetSceneManager()->GetCurrentScene();
        if(!scene)
            return false;

        auto& registry = scene->GetRegistry();
        auto view      = registry.view<ModelComponent>();
        bool found     = false;

        // Evict from asset manager cache once so LoadFromLibrary reloads fresh
        Application::Get().GetAssetManager()->RemoveAsset(Str8StdS(sourcePath));

        for(auto entity : view)
        {
            auto& comp = view.get<ModelComponent>(entity);
            if(comp.ModelRef && comp.ModelRef->GetFilePath() == sourcePath)
            {
                comp.LoadFromLibrary(sourcePath);
                found = true;
            }
        }
        return found;
    }
}
