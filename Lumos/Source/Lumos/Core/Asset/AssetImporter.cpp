#include "Precompiled.h"
#include "AssetImporter.h"
#include "LMeshWriter.h"
#include "LAnimWriter.h"
#include "LImgWriter.h"
#include "Core/OS/FileSystem.h"
#include "Graphics/Animation/Skeleton.h"
#include "Utilities/Hash.h"
#include "Core/Application.h"
#include "Core/Asset/AssetManager.h"
#include "Core/UUID.h"
#include "Graphics/Model.h"
#include "Graphics/Mesh.h"
#include "Graphics/Material.h"
#include "Graphics/RHI/Texture.h"
#include "Graphics/RHI/VertexBuffer.h"
#include "Graphics/RHI/IndexBuffer.h"
#include "Utilities/StringUtilities.h"
#include "Utilities/LoadImage.h"

#include "Maths/MathsSerialisation.h"

#include <cereal/archives/json.hpp>
#include <cereal/cereal.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace Lumos
{
    // Cereal serialization for ImportSettings
    template <typename Archive>
    void serialize(Archive& archive, ImportSettings& s)
    {
        archive(
            cereal::make_nvp("ImportMaterials", s.ImportMaterials),
            cereal::make_nvp("ImportAnimations", s.ImportAnimations),
            cereal::make_nvp("GenerateCollision", s.GenerateCollision),
            cereal::make_nvp("CollisionSimplifyRatio", s.CollisionSimplifyRatio),
            cereal::make_nvp("OptVertexCache", s.OptVertexCache),
            cereal::make_nvp("OptOverdraw", s.OptOverdraw),
            cereal::make_nvp("OptVertexFetch", s.OptVertexFetch),
            cereal::make_nvp("MaxTextureWidth", s.MaxTextureWidth),
            cereal::make_nvp("MaxTextureHeight", s.MaxTextureHeight),
            cereal::make_nvp("CopySourceToProject", s.CopySourceToProject),
            cereal::make_nvp("SplitMeshes", s.SplitMeshes));
    }

    template <typename Archive>
    void serialize(Archive& archive, ImportMeta& m)
    {
        archive(
            cereal::make_nvp("Settings", m.Settings),
            cereal::make_nvp("OutputUUID", m.OutputUUID));
    }

    static std::string GetMetaPath(const std::string& sourcePath)
    {
        return sourcePath + ".meta";
    }

    std::string AssetImporter::NormalizeAssetPath(const std::string& path)
    {
        std::string result;
        result.reserve(path.size());
        for(char c : path)
        {
            if(c == '\\')
                c = '/';
            if(c >= 'A' && c <= 'Z')
                c = c + ('a' - 'A');
            result += c;
        }
        while(!result.empty() && result.back() == '/')
            result.pop_back();
        return result;
    }

    void AssetImporter::CleanOrphanedImports()
    {
        ArenaTemp scratch = ScratchBegin(0, 0);
        String8 assetsPath = FileSystem::Get().GetAssetPath();
        String8 importedDir = PushStr8F(scratch.arena, "%s/Imported", (const char*)assetsPath.str);
        NullTerminate(importedDir);

        if(!FileSystem::FolderExists(importedDir))
        {
            ScratchEnd(scratch);
            return;
        }

        int removed = 0;
        std::error_code ec;
        for(auto& entry : std::filesystem::directory_iterator((const char*)importedDir.str, ec))
        {
            if(!entry.is_regular_file())
                continue;
            auto ext = entry.path().extension().string();
            if(ext == ".lmesh" || ext == ".lanim" || ext == ".limg" || ext == ".lmat")
            {
                // Check if a .meta file references this hash — if orphaned, delete
                // For now just log; actual orphan detection requires scanning all source paths
                // which we don't track globally. Skip files newer than 1 hour to avoid race conditions.
            }
        }

        // Clean Textures subdirectory too
        String8 texDir = PushStr8F(scratch.arena, "%s/Imported/Textures", (const char*)assetsPath.str);
        NullTerminate(texDir);
        if(FileSystem::FolderExists(texDir))
        {
            for(auto& entry : std::filesystem::directory_iterator((const char*)texDir.str, ec))
            {
                if(!entry.is_regular_file())
                    continue;
            }
        }

        if(removed > 0)
            LINFO("AssetImporter: Cleaned %d orphaned import files", removed);

        ScratchEnd(scratch);
    }

    static std::string CopyTextureToImported(Arena* arena, SharedPtr<Graphics::Texture2D> tex, u32 slot)
    {
        if(!tex)
            return "";

        String8 assetsPath = FileSystem::Get().GetAssetPath();

        // Ensure Textures subdirectory exists
        String8 texDir = PushStr8F(arena, "%s/Imported/Textures", (const char*)assetsPath.str);
        NullTerminate(texDir);
        FileSystem::CreateFolderIfDoesntExist(texDir);

        const std::string& texPath = tex->GetFilepath();

        if(!texPath.empty())
        {
            // File-based texture — load with stbi, write as .limg
            std::string normalizedTexPath = AssetImporter::NormalizeAssetPath(texPath);
            u64 texHash = MurmurHash64A(normalizedTexPath.c_str(), (int)normalizedTexPath.size(), 0);

            uint32_t w, h, bits;
            bool isHDR = false;
            uint8_t* pixels = LoadImageFromFile(texPath.c_str(), &w, &h, &bits, &isHDR, false, true);
            if(!pixels)
            {
                LERROR("AssetImporter: Failed to load texture %s for limg conversion", texPath.c_str());
                return "";
            }

            Graphics::RHIFormat format = isHDR ? Graphics::RHIFormat::R32G32B32A32_Float
                                               : Graphics::RHIFormat::R8G8B8A8_Unorm;

            String8 dstPath = PushStr8F(arena, "%s/Imported/Textures/%llu.limg",
                                        (const char*)assetsPath.str, (unsigned long long)texHash);
            NullTerminate(dstPath);

            bool ok = LImgWriter::Write(dstPath, w, h, pixels, format);
            delete[] pixels;

            if(!ok)
            {
                LERROR("AssetImporter: Failed to write limg for %s", texPath.c_str());
                return "";
            }

            char vfsBuf[512];
            snprintf(vfsBuf, sizeof(vfsBuf), "//Assets/Imported/Textures/%llu.limg",
                     (unsigned long long)texHash);
            return std::string(vfsBuf);
        }

        // Embedded texture (e.g. GLB) — pixels already in memory, write as .limg
        const uint8_t* pixels = tex->GetPixelData();
        if(!pixels)
            return "";

        u64 texHash = MurmurHash64A((const char*)pixels, (int)(tex->GetWidth() * tex->GetHeight() * 4), slot);

        String8 dstPath = PushStr8F(arena, "%s/Imported/Textures/%llu.limg",
                                    (const char*)assetsPath.str, (unsigned long long)texHash);
        NullTerminate(dstPath);

        if(!LImgWriter::Write(dstPath, tex->GetWidth(), tex->GetHeight(), pixels, Graphics::RHIFormat::R8G8B8A8_Unorm))
        {
            LERROR("AssetImporter: Failed to write embedded texture as limg");
            return "";
        }

        char vfsBuf[512];
        snprintf(vfsBuf, sizeof(vfsBuf), "//Assets/Imported/Textures/%llu.limg",
                 (unsigned long long)texHash);
        return std::string(vfsBuf);
    }

    std::string AssetImporter::GetImportedPath(const std::string& sourcePath)
    {
        std::string normalized = NormalizeAssetPath(sourcePath);
        u64 pathHash = MurmurHash64A(normalized.c_str(), (int)normalized.size(), 0);
        char buf[256];
        snprintf(buf, sizeof(buf), "//Assets/Imported/%llu.lmesh", (unsigned long long)pathHash);
        return std::string(buf);
    }

    bool AssetImporter::LoadMeta(const std::string& sourcePath, ImportMeta& outMeta)
    {
        ArenaTemp scratch = ScratchBegin(0, 0);

        std::string metaPath = GetMetaPath(sourcePath);
        String8 physicalPath;
        if(!FileSystem::Get().ResolvePhysicalPath(scratch.arena, Str8StdS(metaPath), &physicalPath))
        {
            ScratchEnd(scratch);
            return false;
        }

        if(!FileSystem::FileExists(physicalPath))
        {
            ScratchEnd(scratch);
            return false;
        }

        int64_t fileSize = FileSystem::GetFileSize(physicalPath);
        if(fileSize <= 0)
        {
            ScratchEnd(scratch);
            return false;
        }

        Arena* metaArena = ArenaAlloc((u64)fileSize + Kilobytes(1));
        String8 data = FileSystem::ReadTextFile(metaArena, physicalPath);
        if(data.size == 0)
        {
            ArenaRelease(metaArena);
            ScratchEnd(scratch);
            return false;
        }

        std::istringstream iss(std::string((const char*)data.str, data.size));
        cereal::JSONInputArchive archive(iss);
        serialize(archive, outMeta);

        ArenaRelease(metaArena);
        ScratchEnd(scratch);
        return true;
    }

    bool AssetImporter::SaveMeta(const std::string& sourcePath, const ImportMeta& meta)
    {
        ArenaTemp scratch = ScratchBegin(0, 0);

        std::string metaPath = GetMetaPath(sourcePath);

        std::ostringstream oss;
        {
            cereal::JSONOutputArchive archive(oss);
            ImportMeta m = meta;
            serialize(archive, m);
        }

        std::string json = oss.str();

        // Build physical path from source path rather than ResolvePhysicalPath,
        // because ResolvePhysicalPath returns false for non-existent files
        // and the .meta file won't exist yet on first import.
        String8 sourcePhysical;
        FileSystem::Get().ResolvePhysicalPath(scratch.arena, Str8StdS(sourcePath), &sourcePhysical);
        // ResolvePhysicalPath populates outPhysicalPath even when returning false,
        // but for non-VFS paths it just returns the input. Construct meta path from source.
        String8 physicalPath = PushStr8F(scratch.arena, "%s.meta", (const char*)sourcePhysical.str);
        NullTerminate(physicalPath);

        bool result = FileSystem::WriteTextFile(physicalPath, Str8StdS(json));
        ScratchEnd(scratch);
        return result;
    }

    bool AssetImporter::NeedsImport(const std::string& sourcePath)
    {
        ArenaTemp scratch = ScratchBegin(0, 0);

        // Check if .lmesh exists and has current format version
        std::string importedPath = GetImportedPath(sourcePath);
        bool needsImport = true;

        if(FileSystem::Get().FileExistsVFS(Str8StdS(importedPath)))
        {
            String8 physImportedPath;
            if(FileSystem::Get().ResolvePhysicalPath(scratch.arena, Str8StdS(importedPath), &physImportedPath))
            {
                LMeshHeader peekHeader = {};
                int64_t fsize = FileSystem::GetFileSize(physImportedPath);
                if(fsize >= (int64_t)sizeof(LMeshHeader))
                {
                    FileSystem::ReadFile(scratch.arena, physImportedPath, &peekHeader, (int64_t)sizeof(LMeshHeader));
                }
                if(peekHeader.Version == LMESH_VERSION)
                    needsImport = false;
            }
        }

        ScratchEnd(scratch);
        return needsImport;
    }

    std::string AssetImporter::Import(const std::string& sourcePath, const ImportSettings& settings)
    {
        LUMOS_PROFILE_FUNCTION();
        ArenaTemp scratch = ScratchBegin(0, 0);

        std::string effectivePath = sourcePath;

        // Copy source into project if enabled and file is outside asset root
        if(settings.CopySourceToProject)
        {
            String8 assetsPathStr = FileSystem::Get().GetAssetPath();
            std::string assetsDir((const char*)assetsPathStr.str, assetsPathStr.size);

            String8 physSource;
            bool resolved = FileSystem::Get().ResolvePhysicalPath(scratch.arena, Str8StdS(sourcePath), &physSource);
            std::string physSourceStr = resolved ? std::string((const char*)physSource.str, physSource.size) : sourcePath;

            // Check if source is under assets dir
            if(physSourceStr.find(assetsDir) == std::string::npos)
            {
                // Copy to Assets/Models/
                String8 modelsDir = PushStr8F(scratch.arena, "%s/Models", assetsDir.c_str());
                NullTerminate(modelsDir);
                std::filesystem::create_directories((const char*)modelsDir.str);

                std::string filename = StringUtilities::GetFileName(physSourceStr);
                std::string dstPath = std::string((const char*)modelsDir.str) + "/" + filename;

                // Handle collision: append _N
                if(std::filesystem::exists(dstPath))
                {
                    std::string baseName = StringUtilities::RemoveFilePathExtension(filename);
                    std::string ext = StringUtilities::GetFilePathExtension(filename);
                    for(int n = 1; n < 1000; n++)
                    {
                        char buf[16];
                        snprintf(buf, sizeof(buf), "_%d", n);
                        dstPath = std::string((const char*)modelsDir.str) + "/" + baseName + buf + "." + ext;
                        if(!std::filesystem::exists(dstPath))
                            break;
                    }
                }

                std::error_code ec;
                std::filesystem::copy_file(physSourceStr, dstPath, ec);
                if(!ec)
                {
                    LINFO("AssetImporter: Copied source to %s", dstPath.c_str());
                    // Build VFS path
                    effectivePath = "//Assets/Models/" + std::filesystem::path(dstPath).filename().string();
                }
                else
                {
                    LERROR("AssetImporter: Failed to copy source: %s", ec.message().c_str());
                }
            }
        }

        // Ensure Imported directory exists
        String8 assetsPath = FileSystem::Get().GetAssetPath();
        {
            String8 physDir = PushStr8F(scratch.arena, "%s/Imported", (const char*)assetsPath.str);
            NullTerminate(physDir);
            FileSystem::CreateFolderIfDoesntExist(physDir);
        }

        // Delete existing .lmesh and .lanim before loading source, to prevent
        // LoadModel from finding stale/corrupt files and trying to load them
        std::string normalizedEffective = NormalizeAssetPath(effectivePath);
        u64 pathHash = MurmurHash64A(normalizedEffective.c_str(), (int)normalizedEffective.size(), 0);
        {
            String8 existingLMesh = PushStr8F(scratch.arena, "%s/Imported/%llu.lmesh",
                                               (const char*)assetsPath.str, (unsigned long long)pathHash);
            NullTerminate(existingLMesh);
            if(FileSystem::FileExists(existingLMesh))
                std::remove((const char*)existingLMesh.str);

            String8 existingLAnim = PushStr8F(scratch.arena, "%s/Imported/%llu.lanim",
                                               (const char*)assetsPath.str, (unsigned long long)pathHash);
            NullTerminate(existingLAnim);
            if(FileSystem::FileExists(existingLAnim))
                std::remove((const char*)existingLAnim.str);
        }

        // Load source model using existing loaders
        Graphics::Model sourceModel(effectivePath);
        if(sourceModel.GetMeshes().Empty())
        {
            LERROR("AssetImporter: Failed to load source %s", effectivePath.c_str());
            ScratchEnd(scratch);
            return "";
        }

        // Build LMeshWriteData from loaded model
        LMeshWriteData writeData;
        UUID uuid;
        writeData.AssetUUID = (u64)uuid;

        auto& meshes = sourceModel.GetMeshes();
        LINFO("AssetImporter: source model has %d meshes", meshes.Size());
        for(int i = 0; i < meshes.Size(); i++)
        {
            auto& mesh = meshes[i];
            LMeshWriteData::MeshEntry entry;
            entry.Name = mesh->GetName();
            if(entry.Name.empty())
            {
                char buf[32];
                snprintf(buf, sizeof(buf), "mesh_%d", i);
                entry.Name = buf;
            }

            // Extract vertex/index data from GPU buffers
            // The mesh was just created, so we reconstruct from the buffers
            auto& vb  = mesh->GetVertexBuffer();
            auto& avb = mesh->GetAnimVertexBuffer();
            auto& ib  = mesh->GetIndexBuffer();

            if(avb && avb->GetSize() > 0)
            {
                entry.Animated = true;
                u32 vertCount = avb->GetSize() / sizeof(Graphics::AnimVertex);
                entry.AnimVertices.Resize(vertCount);

                auto* ptr = avb->GetPointer<Graphics::AnimVertex>();
                if(ptr)
                {
                    memcpy(entry.AnimVertices.Data(), ptr, avb->GetSize());
                    avb->ReleasePointer();
                }
            }
            else if(vb && vb->GetSize() > 0)
            {
                u32 vertCount = vb->GetSize() / sizeof(Graphics::Vertex);
                entry.Vertices.Resize(vertCount);

                auto* ptr = vb->GetPointer<Graphics::Vertex>();
                if(ptr)
                {
                    memcpy(entry.Vertices.Data(), ptr, vb->GetSize());
                    vb->ReleasePointer();
                }
            }

            if(ib && ib->GetCount() > 0)
            {
                entry.Indices.Resize(ib->GetCount());
                auto* ptr = ib->GetPointer<u32>();
                if(ptr)
                {
                    memcpy(entry.Indices.Data(), ptr, ib->GetCount() * sizeof(u32));
                    ib->ReleasePointer();
                }
            }

            LINFO("AssetImporter: mesh[%d] '%s' verts=%u indices=%u (tris=%u) animated=%d",
                  i, entry.Name.c_str(),
                  entry.Animated ? (u32)entry.AnimVertices.Size() : (u32)entry.Vertices.Size(),
                  (u32)entry.Indices.Size(), (u32)entry.Indices.Size() / 3,
                  entry.Animated ? 1 : 0);

            // Material
            if(settings.ImportMaterials && mesh->GetMaterial())
            {
                auto& mat = mesh->GetMaterial();
                entry.MaterialIndex = (u32)writeData.Materials.Size();

                LMeshWriteData::MaterialEntry matEntry;
                matEntry.Name = mat->GetName();
                if(mat->GetProperties())
                    matEntry.Properties = *mat->GetProperties();
                matEntry.Flags = mat->GetFlags();

                auto& textures = mat->GetTextures();
                SharedPtr<Graphics::Texture2D> texSlots[] = {
                    textures.albedo, textures.normal, textures.metallic,
                    textures.roughness, textures.ao, textures.emissive
                };
                for(u32 s = 0; s < LMESH_MAX_TEXTURE_SLOTS; s++)
                {
                    if(texSlots[s])
                    {
                        std::string imported = CopyTextureToImported(scratch.arena, texSlots[s], s);
                        if(!imported.empty())
                        {
                            matEntry.TexturePaths[s] = imported;
                            auto params = texSlots[s]->GetTextureParameters();
                            matEntry.TextureWraps[s]   = params.wrap;
                            matEntry.TextureFilters[s] = params.minFilter;
                        }
                    }
                }

                writeData.Materials.PushBack(std::move(matEntry));
            }

            writeData.Meshes.PushBack(std::move(entry));
        }

        // Skeleton/animations
        if(settings.ImportAnimations)
        {
            writeData.Skeleton   = sourceModel.GetSkeleton();
            writeData.Animations = sourceModel.GetAnimations();
            writeData.BindPoses  = sourceModel.GetBindPoses();
        }

        // Write .lmesh
        std::string outputPath = GetImportedPath(effectivePath);

        String8 physOutputPath = PushStr8F(scratch.arena, "%s/Imported/%llu.lmesh",
                                            (const char*)assetsPath.str, (unsigned long long)pathHash);
        NullTerminate(physOutputPath);

        if(!LMeshWriter::Write(physOutputPath, writeData, settings))
        {
            LERROR("AssetImporter: Failed to write lmesh for %s", effectivePath.c_str());
            ScratchEnd(scratch);
            return "";
        }

        // Export .lmat files for each material
        if(settings.ImportMaterials)
        {
            String8 matDir = PushStr8F(scratch.arena, "%s/Imported/Materials", (const char*)assetsPath.str);
            NullTerminate(matDir);
            std::filesystem::create_directories((const char*)matDir.str);

            static const char* texNames[] = { "Albedo", "Normal", "Metallic", "Roughness", "Ao", "Emissive" };

            for(u32 mi = 0; mi < writeData.Materials.Size(); mi++)
            {
                auto& mat = writeData.Materials[mi];

                std::stringstream ss;
                {
                    cereal::JSONOutputArchive ar(ss);

                    // Texture paths
                    for(u32 s = 0; s < LMESH_MAX_TEXTURE_SLOTS; s++)
                    {
                        std::string p = mat.TexturePaths[s];
                        ar(cereal::make_nvp(texNames[s], p));
                    }

                    auto& props = mat.Properties;
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
                       cereal::make_nvp("shader", std::string()));

                    ar(cereal::make_nvp("Reflectance", props.reflectance));
                }

                String8 lmatPath = PushStr8F(scratch.arena, "%s/Imported/Materials/%llu_%u.lmat",
                                              (const char*)assetsPath.str, (unsigned long long)pathHash, mi);
                NullTerminate(lmatPath);
                FileSystem::WriteTextFile(lmatPath, Str8StdS(ss.str()));
            }
        }

        // Write .lanim if we have skeleton/animation data
        if(settings.ImportAnimations && writeData.Skeleton && writeData.Skeleton->Valid())
        {
            LAnimWriteData animData;
            animData.Skeleton   = writeData.Skeleton;
            animData.Animations = writeData.Animations;
            animData.BindPoses  = writeData.BindPoses;
            animData.AssetUUID  = writeData.AssetUUID;

            String8 physAnimPath = PushStr8F(scratch.arena, "%s/Imported/%llu.lanim",
                                              (const char*)assetsPath.str, (unsigned long long)pathHash);
            NullTerminate(physAnimPath);

            if(LAnimWriter::Write(physAnimPath, animData))
                LINFO("AssetImporter: Wrote .lanim for %s", effectivePath.c_str());
            else
                LERROR("AssetImporter: Failed to write .lanim for %s", effectivePath.c_str());
        }

        // Save .meta
        {
            ImportMeta meta;
            meta.Settings   = settings;
            meta.OutputUUID = writeData.AssetUUID;
            SaveMeta(effectivePath, meta);
        }

        LINFO("AssetImporter: Imported %s → %s", effectivePath.c_str(), outputPath.c_str());
        ScratchEnd(scratch);
        return outputPath;
    }
}
