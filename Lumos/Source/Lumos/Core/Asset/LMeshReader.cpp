#include "Precompiled.h"
#include "LMeshReader.h"
#include "Core/OS/FileSystem.h"
#include "Core/Application.h"
#include "Core/Asset/AssetManager.h"
#include "Graphics/Model.h"
#include "Graphics/Mesh.h"
#include "Graphics/Material.h"
#include "Graphics/RHI/Texture.h"
#include "Graphics/RHI/VertexBuffer.h"
#include "Graphics/RHI/IndexBuffer.h"
#include "Maths/BoundingBox.h"
#include "Utilities/StringUtilities.h"

namespace Lumos
{
    bool LMeshReader::Read(Arena* arena, const String8& path, Graphics::Model& outModel)
    {
        ArenaTemp scratch = ScratchBegin(&arena, 1);

        int64_t fileSize = FileSystem::Get().GetFileSizeVFS(path);
        if(fileSize < (int64_t)sizeof(LMeshHeader))
        {
            LERROR("LMeshReader: File too small or missing %.*s", (int)path.size, path.str);
            ScratchEnd(scratch);
            return false;
        }

        // Allocate arena large enough for the file data
        Arena* fileArena = ArenaAlloc((u64)fileSize + Kilobytes(4));
        u8* fileData = FileSystem::Get().ReadFileVFS(fileArena, path);
        if(!fileData)
        {
            LERROR("LMeshReader: Failed to read %.*s", (int)path.size, path.str);
            ArenaRelease(fileArena);
            ScratchEnd(scratch);
            return false;
        }

        // Validate header
        const LMeshHeader* header = (const LMeshHeader*)fileData;
        if(memcmp(header->Magic, LMESH_MAGIC, 4) != 0)
        {
            LERROR("LMeshReader: Invalid magic in %.*s", (int)path.size, path.str);
            ArenaRelease(fileArena);
            ScratchEnd(scratch);
            return false;
        }

        if(header->Version != LMESH_VERSION)
        {
            LERROR("LMeshReader: Version mismatch (got %u, expected %u)", header->Version, LMESH_VERSION);
            ArenaRelease(fileArena);
            ScratchEnd(scratch);
            return false;
        }

        // Validate header counts against file size
        u64 minSize = sizeof(LMeshHeader) + (u64)header->MeshCount * sizeof(LMeshDescriptor);
        if((u64)fileSize < minSize)
        {
            LERROR("LMeshReader: File too small for %u meshes in %.*s", header->MeshCount, (int)path.size, path.str);
            ArenaRelease(fileArena);
            ScratchEnd(scratch);
            return false;
        }

        if(header->MeshCount == 0)
        {
            LERROR("LMeshReader: no meshes in %.*s", (int)path.size, path.str);
            ArenaRelease(fileArena);
            ScratchEnd(scratch);
            return false;
        }

        // Read descriptors
        const LMeshDescriptor* descriptors = (const LMeshDescriptor*)(fileData + sizeof(LMeshHeader));

        // every mesh's data must lie in the file and match its vertex type (a layout change without a version bump)
        for(u32 i = 0; i < header->MeshCount; i++)
        {
            const LMeshDescriptor& d = descriptors[i];
            const u64 stride         = (d.Flags & LMeshVertexFlag_Animated) ? sizeof(Graphics::AnimVertex) : sizeof(Graphics::Vertex);
            const bool bad           = (u64)d.VertexSize != (u64)d.VertexCount * stride
                           || (u64)d.IndexSize != (u64)d.IndexCount * sizeof(u32)
                           || (u64)d.VertexOffset + d.VertexSize > (u64)fileSize
                           || (u64)d.IndexOffset + d.IndexSize > (u64)fileSize;
            if(bad)
            {
                LERROR("LMeshReader: mesh %u data out of range or wrong vertex size in %.*s", i, (int)path.size, path.str);
                ArenaRelease(fileArena);
                ScratchEnd(scratch);
                return false;
            }
        }

        // Read material data
        u8* matPtr = fileData + sizeof(LMeshHeader) + header->MeshCount * sizeof(LMeshDescriptor);
        u8* fileEnd = fileData + fileSize;

        std::string pathStr((const char*)path.str, path.size);
        std::string lmeshFilename = StringUtilities::GetFileName(pathStr);
        // Remove .lmesh extension
        auto dotPos = lmeshFilename.rfind('.');
        std::string hashStr = (dotPos != std::string::npos) ? lmeshFilename.substr(0, dotPos) : lmeshFilename;

        // Parse materials
        TDArray<SharedPtr<Graphics::Material>> materials;
        for(u32 m = 0; m < header->MaterialCount; m++)
        {
            // Bounds check: need at least 4 bytes for nameLen
            if(matPtr + 4 > fileEnd)
            {
                LERROR("LMeshReader: Material %u truncated (nameLen) in %.*s", m, (int)path.size, path.str);
                ArenaRelease(fileArena);
                ScratchEnd(scratch);
                return false;
            }

            u32 nameLen;
            memcpy(&nameLen, matPtr, 4); matPtr += 4;

            if(nameLen > 4096 || matPtr + nameLen + 1 > fileEnd)
            {
                LERROR("LMeshReader: Material %u bad nameLen %u in %.*s", m, nameLen, (int)path.size, path.str);
                ArenaRelease(fileArena);
                ScratchEnd(scratch);
                return false;
            }

            const char* matName = (const char*)matPtr;
            matPtr += nameLen + 1;

            if(matPtr + sizeof(Graphics::MaterialProperties) > fileEnd)
            {
                LERROR("LMeshReader: Material %u truncated (properties) in %.*s", m, (int)path.size, path.str);
                ArenaRelease(fileArena);
                ScratchEnd(scratch);
                return false;
            }

            Graphics::MaterialProperties props;
            memcpy(&props, matPtr, sizeof(Graphics::MaterialProperties));
            matPtr += sizeof(Graphics::MaterialProperties);

            // Material render flags
            if(matPtr + 4 > fileEnd)
            {
                LERROR("LMeshReader: Material %u truncated (flags) in %.*s", m, (int)path.size, path.str);
                ArenaRelease(fileArena);
                ScratchEnd(scratch);
                return false;
            }

            u32 materialFlags;
            memcpy(&materialFlags, matPtr, 4); matPtr += 4;

            if(matPtr + 4 > fileEnd)
            {
                LERROR("LMeshReader: Material %u truncated (texCount) in %.*s", m, (int)path.size, path.str);
                ArenaRelease(fileArena);
                ScratchEnd(scratch);
                return false;
            }

            u32 texCount;
            memcpy(&texCount, matPtr, 4); matPtr += 4;

            if(texCount > LMESH_MAX_TEXTURE_SLOTS)
            {
                LERROR("LMeshReader: Material %u bad texCount %u in %.*s", m, texCount, (int)path.size, path.str);
                ArenaRelease(fileArena);
                ScratchEnd(scratch);
                return false;
            }

            // Try loading from cached .lmat file first
            std::string lmatPathStr = "//Assets/Imported/Materials/" + hashStr + "_" + std::to_string(m) + ".lmat";
            String8 lmatPath = Str8StdS(lmatPathStr);

            SharedPtr<Graphics::Material> material = Application::Get().GetAssetManager()->LoadMaterialAsset(lmatPath);
            const bool fromLmat                    = material != nullptr;

            bool animated = false;
            for(u32 mi = 0; mi < header->MeshCount; mi++)
            {
                if(descriptors[mi].MaterialIndex == m && (descriptors[mi].Flags & LMeshVertexFlag_Animated))
                {
                    animated = true;
                    break;
                }
            }

            if(!material)
            {
                // Fallback: create material inline from lmesh binary data
                auto shader = Application::Get().GetAssetManager()->GetAssetData(animated ? Str8Lit("ForwardPBRAnim") : Str8Lit("ForwardPBR"));
                // no shader yet is fine: the descriptor set falls back to ForwardPBR when it is made
                SharedPtr<Graphics::Shader> shaderPtr = shader ? shader.As<Graphics::Shader>() : SharedPtr<Graphics::Shader>();
                material = CreateSharedPtr<Graphics::Material>(shaderPtr, props);

                material->SetName(matName);

                Graphics::PBRMataterialTextures textures;

                for(u32 t = 0; t < texCount; t++)
                {
                    if(matPtr + 10 > fileEnd)
                    {
                        LERROR("LMeshReader: Material %u texture %u truncated in %.*s", m, t, (int)path.size, path.str);
                        ArenaRelease(fileArena);
                        ScratchEnd(scratch);
                        return false;
                    }

                    u32 slot;
                    memcpy(&slot, matPtr, 4); matPtr += 4;

                    u8 wrap   = *matPtr++;
                    u8 filter = *matPtr++;

                    u32 pathLen;
                    memcpy(&pathLen, matPtr, 4); matPtr += 4;

                    if(pathLen > 4096 || matPtr + pathLen + 1 > fileEnd)
                    {
                        LERROR("LMeshReader: Material %u texture %u bad pathLen %u in %.*s", m, t, pathLen, (int)path.size, path.str);
                        ArenaRelease(fileArena);
                        ScratchEnd(scratch);
                        return false;
                    }

                    std::string texPath((const char*)matPtr, pathLen);
                    matPtr += pathLen + 1;

                    if(slot < LMESH_MAX_TEXTURE_SLOTS)
                    {
                        Graphics::TextureDesc texDesc;
                        texDesc.wrap      = wrap != 0 ? (Graphics::TextureWrap)wrap : Graphics::TextureWrap::REPEAT;
                        texDesc.minFilter = filter != 0 ? (Graphics::TextureFilter)filter : Graphics::TextureFilter::LINEAR;
                        texDesc.magFilter = texDesc.minFilter;
                        auto tex = Application::Get().GetAssetManager()->LoadTextureAsset(Str8StdS(texPath), false, texDesc);
                        if(tex)
                        {
                            switch(slot)
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
                }

                material->SetTextures(textures);
            }
            else
            {
                // .lmat loaded - still need to skip over texture data in the binary
                for(u32 t = 0; t < texCount; t++)
                {
                    if(matPtr + 10 > fileEnd)
                    {
                        ArenaRelease(fileArena);
                        ScratchEnd(scratch);
                        return false;
                    }

                    matPtr += 4; // slot
                    matPtr += 1; // wrap
                    matPtr += 1; // filter

                    u32 pathLen;
                    memcpy(&pathLen, matPtr, 4); matPtr += 4;

                    if(pathLen > 4096 || matPtr + pathLen + 1 > fileEnd)
                    {
                        ArenaRelease(fileArena);
                        ScratchEnd(scratch);
                        return false;
                    }
                    matPtr += pathLen + 1;
                }

                // an older .lmat has no name and no shader
                if(material->GetName().empty())
                    material->SetName(matName);
                if(animated)
                {
                    auto animShader = Application::Get().GetAssetManager()->GetAssetData(Str8Lit("ForwardPBRAnim"));
                    if(animShader)
                    {
                        auto shaderPtr = animShader.As<Graphics::Shader>();
                        if(material->GetShader() != shaderPtr)
                            material->SetShader(shaderPtr);
                    }
                }
            }

            // an .lmat carries its own flags (and may have been edited); the lmesh copy is for when there is none
            if(!fromLmat)
            {
                material->SetFlag(Graphics::Material::RenderFlags::TWOSIDED, (materialFlags & (u32)Graphics::Material::RenderFlags::TWOSIDED) != 0);
                material->SetFlag(Graphics::Material::RenderFlags::ALPHABLEND, (materialFlags & (u32)Graphics::Material::RenderFlags::ALPHABLEND) != 0);
                material->SetFlag(Graphics::Material::RenderFlags::NOSHADOW, (materialFlags & (u32)Graphics::Material::RenderFlags::NOSHADOW) != 0);
            }

            materials.PushBack(material);
        }

        // mesh names follow the materials
        TDArray<std::string> meshNames;
        for(u32 i = 0; i < header->MeshCount; i++)
        {
            u32 nameLen = 0;
            if(matPtr + 4 > fileEnd)
                break;
            memcpy(&nameLen, matPtr, 4);
            if(nameLen > 4096 || matPtr + 4 + nameLen + 1 > fileEnd)
                break;
            meshNames.PushBack(std::string((const char*)matPtr + 4, nameLen));
            matPtr += 4 + nameLen + 1;
        }

        // Create meshes
        LINFO("LMeshReader: loading %u meshes from %.*s", header->MeshCount, (int)path.size, path.str);
        for(u32 i = 0; i < header->MeshCount; i++)
        {
            const LMeshDescriptor& desc = descriptors[i];
            LINFO("LMeshReader: mesh[%u] verts=%u indices=%u (tris=%u) vertOff=%u idxOff=%u",
                  i, desc.VertexCount, desc.IndexCount, desc.IndexCount / 3,
                  desc.VertexOffset, desc.IndexOffset);
            bool animated = (desc.Flags & LMeshVertexFlag_Animated) != 0;

            SharedPtr<Graphics::Mesh> mesh;

            if(animated)
            {
                TDArray<Graphics::AnimVertex> verts(desc.VertexCount);
                verts.Resize(desc.VertexCount);
                memcpy(verts.Data(), fileData + desc.VertexOffset, desc.VertexSize);

                TDArray<u32> indices(desc.IndexCount);
                indices.Resize(desc.IndexCount);
                memcpy(indices.Data(), fileData + desc.IndexOffset, desc.IndexSize);

                mesh = CreateSharedPtr<Graphics::Mesh>(indices, verts);
            }
            else
            {
                TDArray<Graphics::Vertex> verts(desc.VertexCount);
                verts.Resize(desc.VertexCount);
                memcpy(verts.Data(), fileData + desc.VertexOffset, desc.VertexSize);

                TDArray<u32> indices(desc.IndexCount);
                indices.Resize(desc.IndexCount);
                memcpy(indices.Data(), fileData + desc.IndexOffset, desc.IndexSize);

                mesh = CreateSharedPtr<Graphics::Mesh>(indices, verts);
            }

            if(desc.MaterialIndex < materials.Size())
                mesh->SetMaterial(materials[desc.MaterialIndex]);
            if(i < meshNames.Size())
                mesh->SetName(meshNames[i]);

            outModel.AddMesh(mesh);
        }

        ArenaRelease(fileArena);
        ScratchEnd(scratch);
        return true;
    }
}
