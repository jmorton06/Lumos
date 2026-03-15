#include "Precompiled.h"
#include "Model.h"
#include "Mesh.h"
#include "Material.h"
#include "Utilities/StringUtilities.h"
#include "Utilities/Hash.h"
#include "Core/OS/FileSystem.h"
#include "Core/Application.h"
#include "Core/Asset/AssetImporter.h"
#include "Core/Asset/LMeshReader.h"
#include "Core/Asset/LAnimReader.h"
#include "Animation/Skeleton.h"
#include "Animation/Animation.h"
#include "Animation/AnimationController.h"
#include "Animation/SamplingContext.h"

namespace Lumos::Graphics
{
    Model::Model()
        : m_FilePath()
        , m_PrimitiveType(PrimitiveType::None)
    {
    }

    Model::Model(const std::string& filePath)
        : m_FilePath(filePath)
        , m_PrimitiveType(PrimitiveType::File)
    {
        LoadModel(m_FilePath);
    }

    Model::Model(const SharedPtr<Mesh>& mesh, PrimitiveType type)
        : m_FilePath("Primitive")
        , m_PrimitiveType(type)
    {
        m_Meshes.PushBack(mesh);
    }

    Model::Model(PrimitiveType type)
        : m_FilePath("Primitive")
        , m_PrimitiveType(type)
    {
        m_Meshes.PushBack(SharedPtr<Mesh>(CreatePrimative(type)));
    }

    Model::~Model()
    {
    }

    Model::Model(const Model&)            = default;
    Model& Model::operator=(const Model&) = default;
    Model::Model(Model&&)                 = default;
    Model& Model::operator=(Model&&)      = default;

    bool Model::LoadLMesh(const std::string& path)
    {
        LUMOS_PROFILE_FUNCTION();
        Arena* lmeshArena = ArenaAlloc(Kilobytes(8));
        bool result = LMeshReader::Read(lmeshArena, Str8StdS(path), *this);
        ArenaRelease(lmeshArena);
        return result;
    }

    bool Model::LoadLAnim(const std::string& path)
    {
        LUMOS_PROFILE_FUNCTION();
        Arena* lanimArena = ArenaAlloc(Kilobytes(8));
        bool result = LAnimReader::Read(lanimArena, Str8StdS(path), *this);
        ArenaRelease(lanimArena);
        return result;
    }

    void Model::LoadModel(const std::string& path)
    {
        LUMOS_PROFILE_FUNCTION();
        ArenaTemp Scratch = ScratchBegin(0, 0);

        const std::string fileExtension = StringUtilities::GetFilePathExtension(path);

        // If it's already a .lmesh file, load directly
        if(fileExtension == "lmesh")
        {
            if(LoadLMesh(path))
            {
                // Check for companion .lanim (same path with .lanim extension)
                std::string animPath = path.substr(0, path.size() - 5) + "lanim";
                if(FileSystem::Get().FileExistsVFS(Str8StdS(animPath)))
                    LoadLAnim(animPath);

                LINFO("Loaded Model (lmesh direct) - %s meshes=%d", path.c_str(), (int)m_Meshes.Size());
            }
            else
            {
                LERROR("Model: failed to load lmesh (version mismatch or corrupt), reimport required: %s", path.c_str());
            }
            ScratchEnd(Scratch);
            return;
        }

        // Check for imported .lmesh version
        // Convert source path to imported path: //Assets/Imported/<hash>.lmesh
        {
            std::string normalizedPath = AssetImporter::NormalizeAssetPath(path);
            u64 pathHash = MurmurHash64A(normalizedPath.c_str(), (int)normalizedPath.size(), 0);
            char importedPath[256];
            snprintf(importedPath, sizeof(importedPath), "//Assets/Imported/%llu.lmesh", (unsigned long long)pathHash);

            if(FileSystem::Get().FileExistsVFS(Str8C(importedPath)))
            {
                if(LoadLMesh(importedPath))
                {
                    // Check for companion .lanim
                    char animPath[256];
                    snprintf(animPath, sizeof(animPath), "//Assets/Imported/%llu.lanim", (unsigned long long)pathHash);
                    if(FileSystem::Get().FileExistsVFS(Str8C(animPath)))
                        LoadLAnim(animPath);

                    LINFO("Loaded Model (imported lmesh) - %s meshes=%d", path.c_str(), (int)m_Meshes.Size());
                    ScratchEnd(Scratch);
                    return;
                }
                else if(Application::Get().GetProjectSettings().AutoImportMeshes)
                {
                    // .lmesh exists but failed to load (version mismatch) — reimport from source
                    LINFO("Model: reimporting stale .lmesh for %s", path.c_str());
                    ImportSettings settings;
                    std::string imported = AssetImporter::Import(path, settings);
                    if(!imported.empty() && LoadLMesh(importedPath))
                    {
                        char animPath[256];
                        snprintf(animPath, sizeof(animPath), "//Assets/Imported/%llu.lanim", (unsigned long long)pathHash);
                        if(FileSystem::Get().FileExistsVFS(Str8C(animPath)))
                            LoadLAnim(animPath);

                        LINFO("Loaded Model (reimported lmesh) - %s meshes=%d", path.c_str(), (int)m_Meshes.Size());
                        ScratchEnd(Scratch);
                        return;
                    }
                }
            }
        }

        // Fallback to source loaders
        String8 physicalPath;
        if(!Lumos::FileSystem::Get().ResolvePhysicalPath(Scratch.arena, Str8StdS(path), &physicalPath))
        {
            LINFO("Failed to load Model - %s", path.c_str());
            ScratchEnd(Scratch);
            return;
        }

        std::string resolvedPath = ToStdString(physicalPath);

        if(fileExtension == "obj")
            LoadOBJ(resolvedPath);
        else if(fileExtension == "gltf" || fileExtension == "glb")
            LoadGLTF(resolvedPath);
        else if(fileExtension == "fbx" || fileExtension == "FBX")
            LoadFBX(resolvedPath);
        else
            LERROR("Unsupported File Type : %s", fileExtension.c_str());

        LINFO("Loaded Model (source) - %s meshes=%d", path.c_str(), (int)m_Meshes.Size());
        ScratchEnd(Scratch);
    }

    void Model::UpdateAnimation(const TimeStep& dt)
    {
        if(m_Animation.Empty())
            return;

        if(!m_SamplingContext)
        {
            m_SamplingContext = CreateSharedPtr<SamplingContext>();
        }

        if(!m_AnimationController)
        {
            m_AnimationController = CreateSharedPtr<AnimationController>();
            m_AnimationController->SetSkeleton(m_Skeleton);
            for(auto anim : m_Animation)
            {
                m_AnimationController->AddState(anim->GetName(), anim);
            }
            m_AnimationController->SetBindPoses(m_BindPoses);
        }

        static float time = 0.0f;
        time += (float)dt.GetSeconds();
        m_AnimationController->SetCurrentState(m_CurrentAnimation);
        m_AnimationController->Update(time, *m_SamplingContext.get());
    }

    void Model::UpdateAnimation(const TimeStep& dt, float overrideTime)
    {
        if(m_Animation.Empty())
            return;

        if(!m_SamplingContext)
        {
            m_SamplingContext = CreateSharedPtr<SamplingContext>();
        }

        if(!m_AnimationController)
        {
            m_AnimationController = CreateSharedPtr<AnimationController>();
            m_AnimationController->SetSkeleton(m_Skeleton);
            for(auto anim : m_Animation)
            {
                m_AnimationController->AddState(anim->GetName(), anim);
            }
            m_AnimationController->SetBindPoses(m_BindPoses);
        }

        m_AnimationController->SetCurrentState(m_CurrentAnimation);
        m_AnimationController->Update(overrideTime, *m_SamplingContext.get());
    }

    TDArray<Mat4> Model::GetJointMatrices()
    {
        if(m_Animation.Empty())
            return {};

        auto matrices = m_AnimationController->GetJointMatrices();

        for(int i = 0; i < matrices.Size(); i++)
        {
            // matrices[i] = m_BindPoses[i] * matrices[i];
        }

        return matrices;
    }

    TDArray<SharedPtr<Mesh>>& Model::GetMeshesRef()
    {
        return m_Meshes;
    }
    const TDArray<SharedPtr<Mesh>>& Model::GetMeshes() const
    {
        return m_Meshes;
    }
    void Model::AddMesh(SharedPtr<Mesh> mesh)
    {
        m_Meshes.PushBack(mesh);
    }
    SharedPtr<Skeleton> Model::GetSkeleton() const
    {
        return m_Skeleton;
    }

    const TDArray<SharedPtr<Animation>>& Model::GetAnimations() const
    {
        return m_Animation;
    }
    SharedPtr<SamplingContext> Model::GetSamplingContext() const
    {
        return m_SamplingContext;
    }

    SharedPtr<AnimationController> Model::GetAnimationController() const
    {
        return m_AnimationController;
    }

    const TDArray<Mat4>& Model::GetBindPoses() const
    {
        return m_BindPoses;
    }

    void Model::SetSkeleton(SharedPtr<Skeleton> skeleton)
    {
        m_Skeleton = skeleton;
    }

    void Model::SetAnimations(const TDArray<SharedPtr<Animation>>& animations)
    {
        m_Animation = animations;
    }

    void Model::SetBindPoses(const TDArray<Mat4>& bindPoses)
    {
        m_BindPoses = bindPoses;
    }

    void Model::LoadModelAsync(const std::string& path)
    {
        m_Loading       = true;
        m_FilePath      = path;
        m_PrimitiveType = PrimitiveType::File;

        // Defer all model loading to main thread so caller doesn't block.
        // Import() and LoadModel() both create GPU buffers, so they must run on main thread.
        // This is still beneficial: the caller returns immediately and the model
        // loads on the next frame's main thread dispatch, avoiding frame hitches
        // during scene construction.
        auto* self = this;
        std::string pathCopy = path;

        Application::Get().SubmitToMainThread([self, pathCopy]()
        {
            self->LoadModel(pathCopy);
            self->m_Loading = false;
        });
    }
}
