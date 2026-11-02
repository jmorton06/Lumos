#include "Precompiled.h"
#include "ModelComponent.h"
#include "Graphics/Animation/AnimationController.h"
#include "Core/Application.h"
#include "Core/Asset/AssetManager.h"
#include "Core/Asset/AssetImporter.h"
#include "Core/OS/FileSystem.h"
#include "Graphics/Mesh.h"
#include "Utilities/StringUtilities.h"
#include "Utilities/Hash.h"
#include "Scene/SceneManager.h"
#include "Scene/Scene.h"
#include "Scene/Serialisation/SerialisationImplementation.h"
#include <cereal/archives/json.hpp>
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

    static std::string FindSourceForImportedPath(const std::string& importedLmeshPath)
    {
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

                std::string filePath = entry.path().string();
                std::string ext      = StringUtilities::GetFilePathExtension(filePath);
                if(!IsSourceModelExtension(ext))
                    continue;

                // Skip files inside Imported/ directory
                if(filePath.find("/Imported/") != std::string::npos)
                    continue;

                // Build VFS path: strip assetsDir prefix, prepend //Assets/
                std::string rel = filePath.substr(assetsDir.size());
                for(char& c : rel)
                    if(c == '\\') c = '/';
                if(!rel.empty() && rel[0] == '/')
                    rel = rel.substr(1);
                std::string vfsPath = "//Assets/" + rel;

                // Check new normalized hash
                if(AssetImporter::GetImportedPath(vfsPath) == importedLmeshPath)
                {
                    foundSource = vfsPath;
                    break;
                }

                // Check old hash (pre-normalization)
                u64 oldHash = MurmurHash64A(vfsPath.c_str(), (int)vfsPath.size(), 0);
                char oldImported[256];
                snprintf(oldImported, sizeof(oldImported), "//Assets/Imported/%llu.lmesh", (unsigned long long)oldHash);
                if(std::string(oldImported) == importedLmeshPath)
                {
                    foundSource = vfsPath;
                    break;
                }
            }
        }
        catch(...) {}

        ScratchEnd(scratch);
        return foundSource;
    }

    void ModelComponent::LoadFromLibrary(const std::string& path, bool previewOnly)
    {
        std::string loadPath = path;

        std::string ext = StringUtilities::GetFilePathExtension(path);

        // Check asset cache first — if already loaded, use it directly
        {
            String8 CachePath = Str8StdS(loadPath);
            auto am = Application::Get().GetAssetManager();
            if(am->AssetExists(CachePath))
            {
                ModelRef = am->GetAssetData(CachePath).As<Graphics::Model>();
                return;
            }
        }

        // Auto-import source model files to .lmesh
        if(!previewOnly && Application::Get().GetProjectSettings().AutoImportMeshes && IsSourceModelExtension(ext))
        {
            if(AssetImporter::NeedsImport(path))
            {
                LINFO("Auto-importing source model: %s", path.c_str());
                AssetImporter::Import(path);
            }
        }
        else if(!previewOnly && ext == "lmesh" && path.find("//Assets/Imported/") != std::string::npos)
        {
            std::string foundSource = FindSourceForImportedPath(path);

            if(!foundSource.empty())
            {
                // load by the source path from now on; only rebuild the .lmesh when it is out of date
                LINFO("ModelComponent: imported path -> source %s", foundSource.c_str());
                if(AssetImporter::NeedsImport(foundSource))
                    AssetImporter::Import(foundSource);
                loadPath = foundSource;
            }
            else
            {
                LERROR("ModelComponent: no source found for stale imported path %s", path.c_str());
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

    static const char* PrimitiveTypeNames[] = {
        "Primitive_Plane", "Primitive_Quad", "Primitive_Cube",
        "Primitive_Pyramid", "Primitive_Sphere", "Primitive_Capsule",
        "Primitive_Cylinder", "Primitive_Terrain", "Primitive_File", "Primitive_None"
    };

    void ModelComponent::LoadPrimitive(PrimitiveType primitive)
    {
        int idx = (int)primitive;
        if(idx < 0 || idx >= (int)(sizeof(PrimitiveTypeNames) / sizeof(PrimitiveTypeNames[0])))
        {
            ModelRef = CreateSharedPtr<Graphics::Model>(primitive);
            return;
        }

        String8 key = Str8C((char*)PrimitiveTypeNames[idx]);
        auto am     = Application::Get().GetAssetManager();
        if(am->AssetExists(key))
        {
            ModelRef = am->GetAssetData(key).As<Graphics::Model>();
            return;
        }
        ModelRef = am->AddAsset(key, CreateSharedPtr<Graphics::Model>(primitive)).Data.As<Graphics::Model>();
    }

    void ModelComponent::ApplyMaterialOverrides()
    {
        if(!ModelRef)
            return;
        auto& meshes = ModelRef->GetMeshes();
        for(auto& [idx, path] : MaterialOverrides)
        {
            if(idx >= (u32)meshes.Size() || path.empty())
                continue;
            // one object per file (cached), so every entity wearing it shares it
            SharedPtr<Material> material = Application::Get().GetAssetManager()->LoadMaterialAsset(Str8StdS(path));
            if(material)
                InstanceMaterials[idx] = material;
            else
                LERROR("ModelComponent: could not load material %s", path.c_str());
        }
    }

    void ModelComponent::SetMaterialOverride(u32 mesh, const std::string& path)
    {
        if(path.empty())
        {
            MaterialOverrides.erase(mesh);
            InstanceMaterials.erase(mesh);
            return;
        }
        MaterialOverrides[mesh] = path;
        if(SharedPtr<Material> material = Application::Get().GetAssetManager()->LoadMaterialAsset(Str8StdS(path)))
            InstanceMaterials[mesh] = material;
    }

    bool ModelComponent::WriteMaterialFile(const Material& material, const std::string& path)
    {
        std::ostringstream out;
        {
            cereal::JSONOutputArchive archive(out);
            Graphics::save(archive, material);
        }
        // the cached material (if any) is the one just written, so the cache stays as it is
        return FileSystem::Get().WriteTextFileVFS(Str8StdS(path), Str8StdS(out.str()));
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

    AnimationController* ModelComponent::GetAnimator()
    {
        if(!ModelRef || ModelRef->GetAnimations().Empty())
        {
            Animator = nullptr;
            return nullptr;
        }
        // a different model, or the same one with other clips (an animation override): start again
        if(!Animator || AnimatorSource != ModelRef.get() || AnimatorClipCount != ModelRef->GetAnimations().Size())
        {
            Animator          = ModelRef->CreateAnimator();
            AnimatorSource    = ModelRef.get();
            AnimatorClipCount = ModelRef->GetAnimations().Size();
            AnimatorScripted  = false;
        }
        return Animator.get();
    }

    void ModelComponent::TickAnimation(float dt)
    {
        AnimationController* animator = GetAnimator();
        if(!animator)
            return;
        if(!AnimatorScripted && animator->GetLayerWeight((int)ModelRef->GetCurrentAnimationIndex()) <= 0.0f)
            animator->Play((int)ModelRef->GetCurrentAnimationIndex(), animator->HasLayers() ? 0.2f : 0.0f);
        animator->Tick(dt);
    }
}
