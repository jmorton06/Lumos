#include "Precompiled.h"
#include "Model.h"
#include "Maths/MathsUtilities.h"
#include "Mesh.h"
#include "Material.h"
#include "Utilities/StringUtilities.h"
#include "Utilities/Hash.h"
#include "Core/OS/FileSystem.h"
#include "Core/Application.h"
#include "Core/Asset/AssetImporter.h"
#include "Core/Asset/LMeshReader.h"
#include "Core/Asset/LAnimReader.h"
#include "Graphics/MeshOptimiser.h"
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

    namespace
    {
        // <file>.lmesh.lods: the auto LODs, so they are made once rather than every load
        struct LODCacheHeader
        {
            char Magic[4];
            u32 Version;
            u64 Key;
            u32 Levels;
            u32 Meshes;
        };
        static constexpr u32 LOD_CACHE_VERSION = 3; // 2: shared meshes stored as references, 3: levels skip <15% savings

        // what the meshes are (counts, not the file: a packed .lmesh has another size) + the LOD settings
        u64 LODCacheKey(const TDArray<SharedPtr<Mesh>>& meshes)
        {
            u64 counts[3] = { (u64)meshes.Size(), 0, 0 };
            for(auto& mesh : meshes)
                if(mesh)
                {
                    counts[1] += mesh->GetCPUIndices().Size();
                    counts[2] += mesh->GetCPUPositions().Size();
                }
            // hashed field by field: a struct's padding bytes are garbage
            u64 k[10] = { counts[0], counts[1], counts[2], Model::s_AutoLODLevels, Model::s_AutoLODSloppyLast ? 1u : 0u, (u64)sizeof(PackedVertex) };
            const float f[4] = { Model::s_AutoLODRatio, Model::s_AutoLODError, Model::s_AutoLODFirstScreen, Model::s_AutoMergeCell };
            memcpy(&k[6], f, sizeof(f));
            return MurmurHash64A(k, (int)sizeof(k), 0x10D5);
        }
    }

    bool Model::LoadLMesh(const std::string& path)
    {
        LUMOS_PROFILE_FUNCTION();
        Arena* lmeshArena = ArenaAlloc(Kilobytes(8));
        const bool autoLOD    = s_AutoLODLevels > 0;
        const bool autoMerge  = s_AutoMergeCell > 0.0f;
        const std::string lodCache = path + ".lods";
        const bool lodCached  = autoLOD && FileSystem::Get().FileExistsVFS(Str8StdS(lodCache));
        const bool prevRetain = Mesh::GetRetainCPUVertices();
        // CPU vertices only when something has to be built from them
        if(autoMerge || (autoLOD && !lodCached))
            Mesh::SetRetainCPUVertices(true);
        bool result = LMeshReader::Read(lmeshArena, Str8StdS(path), *this);
        ArenaRelease(lmeshArena);
        if(result && autoMerge)
        {
            Mesh::SetRetainCPUVertices(autoLOD || prevRetain);
            Graphics::MeshOptimiser::MergeByMaterial(*this, s_AutoMergeCell);
        }
        Mesh::SetRetainCPUVertices(prevRetain);
        const u64 lodKey = result && autoLOD ? LODCacheKey(m_Meshes) : 0;
        if(result && autoLOD && !(lodCached && LoadLODCache(lodCache, lodKey)))
        {
            if(lodCached && !autoMerge)
            {
                // stale cache: the meshes were read without the CPU copies the simplifier needs
                m_Meshes.Clear();
                ClearLODs();
                Mesh::SetRetainCPUVertices(true);
                Arena* again = ArenaAlloc(Kilobytes(8));
                result       = LMeshReader::Read(again, Str8StdS(path), *this);
                ArenaRelease(again);
                Mesh::SetRetainCPUVertices(prevRetain);
            }
            if(result)
                AutoGenerateLODs(lodCache, lodKey);
        }
        if(result && !prevRetain)
            for(auto& mesh : m_Meshes)
                if(mesh)
                    mesh->DropCPUVertices();
        return result;
    }

    bool Model::LoadLODCache(const std::string& cachePath, u64 cacheKey)
    {
        LUMOS_PROFILE_FUNCTION();
        const int64_t size = FileSystem::Get().GetFileSizeVFS(Str8StdS(cachePath));
        if(size < (int64_t)sizeof(LODCacheHeader))
            return false;
        Arena* arena      = ArenaAlloc((u64)size + Kilobytes(64));
        const u8* data    = FileSystem::Get().ReadFileVFS(arena, Str8StdS(cachePath));
        bool ok           = false;
        if(data)
        {
            LODCacheHeader h;
            memcpy(&h, data, sizeof(h));
            ok = memcmp(h.Magic, "LLOD", 4) == 0 && h.Version == LOD_CACHE_VERSION && h.Key == cacheKey && h.Meshes == m_Meshes.Size();
            if(!ok)
                LINFO("Model LODs: %s is stale (version %u, key %llx vs %llx, meshes %u vs %u)", cachePath.c_str(), h.Version,
                      (unsigned long long)h.Key, (unsigned long long)cacheKey, h.Meshes, (u32)m_Meshes.Size());
            const u8* p   = data + sizeof(h);
            const u8* end = data + size;
            TDArray<LODLevel> levels;
            for(u32 l = 0; ok && l < h.Levels; l++)
            {
                LODLevel level;
                if(p + 8 > end) { ok = false; break; }
                memcpy(&level.ScreenSize, p, 4);
                memcpy(&level.Error, p + 4, 4);
                p += 8;
                for(u32 m = 0; ok && m < h.Meshes; m++)
                {
                    u32 counts[2];
                    if(p + 8 > end) { ok = false; break; }
                    memcpy(counts, p, 8);
                    p += 8;
                    if(counts[0] == ~0u)
                    {
                        if(counts[1] > l) { ok = false; break; }
                        level.Meshes.PushBack(counts[1] == 0 ? m_Meshes[m] : levels[counts[1] - 1].Meshes[m]);
                        continue;
                    }
                    const u64 bytes = (u64)counts[0] * sizeof(PackedVertex) + (u64)counts[1] * 4;
                    if(p + bytes > end) { ok = false; break; }
                    TDArray<Vertex> verts;
                    verts.Resize(counts[0]);
                    const PackedVertex* pv = (const PackedVertex*)p;
                    for(u32 v = 0; v < counts[0]; v++)
                        verts[v] = UnpackVertex(pv[v]);
                    p += (u64)counts[0] * sizeof(PackedVertex);
                    TDArray<u32> indices;
                    indices.Resize(counts[1]);
                    memcpy(indices.Data(), p, (size_t)counts[1] * 4);
                    p += (u64)counts[1] * 4;
                    auto mesh = CreateSharedPtr<Mesh>(indices, verts);
                    mesh->SetName(m_Meshes[m]->GetName());
                    mesh->SetMaterial(m_Meshes[m]->GetMaterial());
                    level.Meshes.PushBack(mesh);
                }
                levels.PushBack(level);
            }
            if(!ok)
                LINFO("Model LODs: %s did not read back", cachePath.c_str());
            if(ok)
            {
                ClearLODs();
                for(auto& level : levels)
                    AddLOD(level.Meshes, level.ScreenSize, level.Error);
                LINFO("Model LODs: %u levels from %s", h.Levels, cachePath.c_str());
            }
        }
        ArenaRelease(arena);
        return ok;
    }

    void Model::AutoGenerateLODs(const std::string& cachePath, u64 cacheKey)
    {
        u32 triangles = 0;
        for(auto& mesh : m_Meshes)
        {
            if(!mesh || mesh->GetAnimVertexBuffer())
                return; // skinned models keep one level
            triangles += (u32)mesh->GetCPUIndices().Size() / 3;
        }

        // small models are not worth the load time
        if(triangles >= 1000)
        {
            Graphics::SimplifySettings settings;
            settings.TargetError = s_AutoLODError;
            // first level once a mesh covers under s_AutoLODFirstScreen/2 of the half-view height, then half that...
            const u32 made = Graphics::MeshOptimiser::GenerateLODs(*this, s_AutoLODLevels, s_AutoLODRatio, settings, s_AutoLODFirstScreen, s_AutoLODSloppyLast);
            u32 lodTris    = 0;
            for(auto& mesh : GetLODMeshes(GetLODCount() - 1))
                if(mesh)
                    lodTris += (u32)mesh->GetCPUIndices().Size() / 3;
            LINFO("Model LODs: %u levels, %u -> %u triangles", made, triangles, lodTris);

            // keep them for next time (a read-only bundle just makes them again)
            if(made > 0 && !cachePath.empty())
            {
                // the level (0 = the model itself) this level's mesh m is shared with, ~0u if its own
                auto LODReuse = [this](u32 l, u32 m) -> u32
                {
                    const auto& mesh = GetLODLevel(l)->Meshes[m];
                    for(u32 prev = 0; prev < l; prev++)
                        if(mesh == (prev == 0 ? m_Meshes[m] : GetLODLevel(prev)->Meshes[m]))
                            return prev;
                    return ~0u;
                };
                // sized up front: growing a TDArray a piece at a time copies the lot each time
                u64 total = sizeof(LODCacheHeader);
                for(u32 l = 1; l <= made; l++)
                {
                    total += 8;
                    const auto& meshes = GetLODLevel(l)->Meshes;
                    for(u32 m = 0; m < meshes.Size(); m++)
                        total += 8 + (LODReuse(l, m) != ~0u || !meshes[m] ? 0 : (u64)meshes[m]->GetCPUVertices().Size() * sizeof(PackedVertex) + (u64)meshes[m]->GetCPUIndices().Size() * 4);
                }
                TDArray<u8> out;
                out.Resize((u32)total);
                u64 at   = 0;
                auto put = [&out, &at](const void* src, size_t n)
                {
                    memcpy(out.Data() + at, src, n);
                    at += n;
                };
                LODCacheHeader h = { { 'L', 'L', 'O', 'D' }, LOD_CACHE_VERSION, cacheKey, made, (u32)m_Meshes.Size() };
                put(&h, sizeof(h));
                bool complete = true;
                for(u32 l = 1; l <= made && complete; l++)
                {
                    const LODLevel* level = GetLODLevel(l);
                    put(&level->ScreenSize, 4);
                    put(&level->Error, 4);
                    for(u32 m = 0; m < level->Meshes.Size(); m++)
                    {
                        auto& mesh = level->Meshes[m];
                        // too small to reduce: the level before's mesh again, kept as a reference
                        const u32 reuse = LODReuse(l, m);
                        if(reuse != ~0u)
                        {
                            u32 ref[2] = { ~0u, reuse };
                            put(ref, 8);
                            continue;
                        }
                        if(!mesh || !mesh->HasCPUVertices())
                        {
                            complete = false;
                            break;
                        }
                        const auto& verts = mesh->GetCPUVertices();
                        const auto& idx   = mesh->GetCPUIndices();
                        u32 counts[2]     = { (u32)verts.Size(), (u32)idx.Size() };
                        put(counts, 8);
                        TDArray<PackedVertex> packed;
                        packed.Resize(counts[0]);
                        PackVertices(verts.Data(), counts[0], packed.Data());
                        put(packed.Data(), (size_t)counts[0] * sizeof(PackedVertex));
                        put(idx.Data(), (size_t)counts[1] * 4);
                    }
                }
                if(!complete || at != total)
                    LWARN("Model LODs: not cached (%s)", complete ? "size mismatch" : "a level has no CPU vertices");
                else if(FileSystem::Get().WriteFileVFS(Str8StdS(cachePath), out.Data(), (u32)total))
                    LINFO("Model LODs saved to %s (%.1f MB)", cachePath.c_str(), total / (1024.0 * 1024.0));
                else
                    LWARN("Model LODs: could not write %s", cachePath.c_str());
            }
        }

        for(u32 l = 1; l < GetLODCount(); l++)
            for(auto& mesh : GetLODMeshes(l))
                if(mesh)
                    mesh->DropCPUVertices();
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
                    std::string imported = AssetImporter::Import(path);
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

        LoadFromSource(path);
        ScratchEnd(Scratch);
    }

    bool Model::LoadFromSource(const std::string& path)
    {
        LUMOS_PROFILE_FUNCTION();
        ArenaTemp Scratch = ScratchBegin(0, 0);

        String8 physicalPath;
        if(!Lumos::FileSystem::Get().ResolvePhysicalPath(Scratch.arena, Str8StdS(path), &physicalPath))
        {
            LINFO("Failed to load Model - %s", path.c_str());
            ScratchEnd(Scratch);
            return false;
        }

        const std::string fileExtension = StringUtilities::GetFilePathExtension(path);
        std::string resolvedPath        = ToStdString(physicalPath);

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
        return !m_Meshes.Empty();
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

        m_AnimationTime += (float)dt.GetSeconds();
        m_AnimationController->SetCurrentState(m_CurrentAnimation);
        m_AnimationController->Update(m_AnimationTime, *m_SamplingContext.get());
    }

    SharedPtr<AnimationController> Model::CreateAnimator() const
    {
        if(m_Animation.Empty() || !m_Skeleton)
            return nullptr;
        auto animator = CreateSharedPtr<AnimationController>();
        animator->SetSkeleton(m_Skeleton);
        for(auto& anim : m_Animation)
            animator->AddState(anim->GetName(), anim);
        animator->SetBindPoses(m_BindPoses);
        return animator;
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

    const TDArray<SharedPtr<Mesh>>& Model::GetLODMeshes(u32 lod) const
    {
        if(lod == 0 || lod > m_LODs.Size())
            return m_Meshes;
        return m_LODs[lod - 1].Meshes;
    }

    Model::LODLevel* Model::GetLODLevel(u32 lod)
    {
        return (lod >= 1 && lod <= m_LODs.Size()) ? &m_LODs[lod - 1] : nullptr;
    }

    void Model::ClearLODs()
    {
        m_LODs.Clear();
    }

    void Model::AddLOD(const TDArray<SharedPtr<Mesh>>& meshes, float screenSize, float error)
    {
        LODLevel level;
        level.Meshes     = meshes;
        level.ScreenSize = screenSize;
        level.Error      = error;
        m_LODs.PushBack(level);
    }

    u32 Model::SelectLOD(float screenSize) const
    {
        u32 lod = 0;
        if(m_ForcedLOD >= 0)
            lod = Maths::Min((u32)m_ForcedLOD, (u32)m_LODs.Size());
        else
        {
            for(u32 i = 0; i < m_LODs.Size(); i++)
            {
                if(screenSize < m_LODs[i].ScreenSize)
                    lod = i + 1;
            }
        }
        m_LastSelectedLOD = lod;
        return lod;
    }

    void Model::GetBoundingSphere(Vec3& centre, float& radius) const
    {
        Maths::BoundingBox box;
        bool any = false;
        for(auto& mesh : m_Meshes)
            if(mesh)
            {
                box.Merge(mesh->GetBoundingBox());
                any = true;
            }
        if(!any)
        {
            centre = Vec3(0.0f);
            radius = 0.0f;
            return;
        }
        centre = box.Center();
        radius = Maths::Length(box.Size()) * 0.5f;
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

        auto* self = this;
        std::string pathCopy = path;

        Application::Get().SubmitToMainThread([self, pathCopy]()
        {
            self->LoadModel(pathCopy);
            self->m_Loading = false;
        });
    }
}
