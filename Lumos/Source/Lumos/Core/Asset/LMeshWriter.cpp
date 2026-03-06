#include "Precompiled.h"
#include "LMeshWriter.h"
#include "AssetImporter.h"
#include "Core/OS/FileSystem.h"
#include "Utilities/Hash.h"
#include "Graphics/Material.h"
#include "Graphics/Animation/Skeleton.h"
#include "Graphics/Animation/Animation.h"
#include "Maths/MathsUtilities.h"

#include <ModelLoaders/meshoptimizer/src/meshoptimizer.h>

namespace Lumos
{
    void LMeshWriter::OptimizeMesh(LMeshWriteData::MeshEntry& mesh, const ImportSettings& settings)
    {
        if(mesh.Animated)
        {
            u32 indexCount  = (u32)mesh.Indices.Size();
            u32 vertexCount = (u32)mesh.AnimVertices.Size();
            if(indexCount == 0 || vertexCount == 0)
                return;

            size_t stride = sizeof(Graphics::AnimVertex);

            if(settings.OptVertexCache)
            {
                TDArray<u32> optimized(indexCount);
                optimized.Resize(indexCount);
                meshopt_optimizeVertexCache(optimized.Data(), mesh.Indices.Data(), indexCount, vertexCount);
                mesh.Indices = std::move(optimized);
            }

            if(settings.OptOverdraw)
            {
                TDArray<u32> optimized(indexCount);
                optimized.Resize(indexCount);
                meshopt_optimizeOverdraw(optimized.Data(), mesh.Indices.Data(), indexCount,
                                         &mesh.AnimVertices[0].Position.x, vertexCount, stride, 1.05f);
                mesh.Indices = std::move(optimized);
            }

            if(settings.OptVertexFetch)
            {
                TDArray<Graphics::AnimVertex> optimizedVerts(vertexCount);
                optimizedVerts.Resize(vertexCount);
                meshopt_optimizeVertexFetch(optimizedVerts.Data(), mesh.Indices.Data(), indexCount,
                                            mesh.AnimVertices.Data(), vertexCount, stride);
                mesh.AnimVertices = std::move(optimizedVerts);
            }
        }
        else
        {
            u32 indexCount  = (u32)mesh.Indices.Size();
            u32 vertexCount = (u32)mesh.Vertices.Size();
            if(indexCount == 0 || vertexCount == 0)
                return;

            size_t stride = sizeof(Graphics::Vertex);

            if(settings.OptVertexCache)
            {
                TDArray<u32> optimized(indexCount);
                optimized.Resize(indexCount);
                meshopt_optimizeVertexCache(optimized.Data(), mesh.Indices.Data(), indexCount, vertexCount);
                mesh.Indices = std::move(optimized);
            }

            if(settings.OptOverdraw)
            {
                TDArray<u32> optimized(indexCount);
                optimized.Resize(indexCount);
                meshopt_optimizeOverdraw(optimized.Data(), mesh.Indices.Data(), indexCount,
                                         &mesh.Vertices[0].Position.x, vertexCount, stride, 1.05f);
                mesh.Indices = std::move(optimized);
            }

            if(settings.OptVertexFetch)
            {
                TDArray<Graphics::Vertex> optimizedVerts(vertexCount);
                optimizedVerts.Resize(vertexCount);
                meshopt_optimizeVertexFetch(optimizedVerts.Data(), mesh.Indices.Data(), indexCount,
                                            mesh.Vertices.Data(), vertexCount, stride);
                mesh.Vertices = std::move(optimizedVerts);
            }
        }
    }

    void LMeshWriter::GenerateCollisionMesh(const LMeshWriteData& data, TDArray<Vec3>& outVerts, TDArray<u32>& outIndices, float simplifyRatio)
    {
        // Gather all positions and indices from all meshes
        u32 vertexOffset = 0;
        for(int i = 0; i < data.Meshes.Size(); i++)
        {
            auto& mesh = data.Meshes[i];
            u32 vertCount = mesh.Animated ? (u32)mesh.AnimVertices.Size() : (u32)mesh.Vertices.Size();

            for(u32 v = 0; v < vertCount; v++)
            {
                Vec3 pos = mesh.Animated ? mesh.AnimVertices[v].Position : mesh.Vertices[v].Position;
                outVerts.PushBack(pos);
            }

            for(int j = 0; j < mesh.Indices.Size(); j++)
                outIndices.PushBack(mesh.Indices[j] + vertexOffset);

            vertexOffset += vertCount;
        }

        if(outIndices.Empty())
            return;

        // Simplify
        size_t targetIndexCount = (size_t)(outIndices.Size() * simplifyRatio);
        if(targetIndexCount < 3)
            targetIndexCount = 3;

        TDArray<u32> simplified((u32)outIndices.Size());
        simplified.Resize((u32)outIndices.Size());

        float resultError = 0.0f;
        size_t finalCount = meshopt_simplify(simplified.Data(), outIndices.Data(), outIndices.Size(),
                                              &outVerts[0].x, outVerts.Size(), sizeof(Vec3),
                                              targetIndexCount, 0.01f, &resultError);

        simplified.Resize((u32)finalCount);
        outIndices = std::move(simplified);
    }

    bool LMeshWriter::Write(const String8& outputPath, LMeshWriteData& data, const ImportSettings& settings)
    {
        for(int i = 0; i < data.Meshes.Size(); i++)
            OptimizeMesh(data.Meshes[i], settings);

        // Log post-optimization counts
        for(int i = 0; i < data.Meshes.Size(); i++)
        {
            auto& m = data.Meshes[i];
            LINFO("LMeshWriter: mesh[%d] '%s' verts=%u indices=%u (tris=%u)",
                  i, m.Name.c_str(),
                  m.Animated ? (u32)m.AnimVertices.Size() : (u32)m.Vertices.Size(),
                  (u32)m.Indices.Size(), (u32)m.Indices.Size() / 3);
        }

        // Calculate layout
        u32 meshCount     = (u32)data.Meshes.Size();
        u32 materialCount = (u32)data.Materials.Size();

        // Header + descriptors
        u32 offset = sizeof(LMeshHeader) + meshCount * sizeof(LMeshDescriptor);

        // Material data (variable size, calculate later)
        u32 materialDataOffset = offset;

        // Pre-calculate material block size
        u32 materialBlockSize = 0;
        for(int i = 0; i < data.Materials.Size(); i++)
        {
            auto& mat = data.Materials[i];
            materialBlockSize += 4; // name length
            materialBlockSize += (u32)mat.Name.size() + 1; // name + null
            materialBlockSize += sizeof(Graphics::MaterialProperties);
            materialBlockSize += 4; // material render flags
            materialBlockSize += 4; // texture count

            for(u32 s = 0; s < LMESH_MAX_TEXTURE_SLOTS; s++)
            {
                if(!mat.TexturePaths[s].empty())
                {
                    materialBlockSize += 4; // slot
                    materialBlockSize += 1; // wrap
                    materialBlockSize += 1; // filter
                    materialBlockSize += 4; // path length
                    materialBlockSize += (u32)mat.TexturePaths[s].size() + 1;
                }
            }
        }

        offset += materialBlockSize;

        // Vertex/index data offsets per mesh
        TDArray<LMeshDescriptor> descriptors;
        for(int i = 0; i < data.Meshes.Size(); i++)
        {
            auto& mesh = data.Meshes[i];
            LMeshDescriptor desc = {};
            desc.NameHash = MurmurHash64A(mesh.Name.c_str(), (int)mesh.Name.size(), 0);
            desc.Flags    = mesh.Animated ? LMeshVertexFlag_Animated : LMeshVertexFlag_None;

            if(mesh.Animated)
            {
                desc.VertexOffset = offset;
                desc.VertexCount  = (u32)mesh.AnimVertices.Size();
                desc.VertexSize   = desc.VertexCount * sizeof(Graphics::AnimVertex);
            }
            else
            {
                desc.VertexOffset = offset;
                desc.VertexCount  = (u32)mesh.Vertices.Size();
                desc.VertexSize   = desc.VertexCount * sizeof(Graphics::Vertex);
            }
            offset += desc.VertexSize;

            desc.IndexOffset = offset;
            desc.IndexCount  = (u32)mesh.Indices.Size();
            desc.IndexSize   = desc.IndexCount * sizeof(u32);
            offset += desc.IndexSize;

            desc.MaterialIndex = mesh.MaterialIndex;
            descriptors.PushBack(desc);
        }

        // Bounding boxes
        u32 boundsOffset = offset;
        offset += meshCount * sizeof(LMeshBounds);

        // Collision mesh
        TDArray<Vec3> collisionVerts;
        TDArray<u32> collisionIndices;
        u32 collisionOffset = 0;

        bool hasCollision = settings.GenerateCollision;
        if(hasCollision)
        {
            GenerateCollisionMesh(data, collisionVerts, collisionIndices, settings.CollisionSimplifyRatio);
            collisionOffset = offset;
            offset += sizeof(LMeshCollisionHeader);
            offset += (u32)collisionVerts.Size() * sizeof(Vec3);
            offset += (u32)collisionIndices.Size() * sizeof(u32);
        }

        // TODO: skeleton + animation data

        u32 totalSize = offset;

        // Build header
        LMeshHeader header = {};
        memcpy(header.Magic, LMESH_MAGIC, 4);
        header.Version       = LMESH_VERSION;
        header.Flags         = 0;
        header.AssetUUID     = data.AssetUUID;
        header.MeshCount     = meshCount;
        header.MaterialCount = materialCount;
        header.SkeletonOffset = 0;
        header.AnimationCount = 0;
        header.TotalFileSize  = totalSize;
        header.Reserved       = 0;

        if(data.Skeleton)
            header.Flags |= LMeshFlag_HasAnimation;
        if(hasCollision)
            header.Flags |= LMeshFlag_HasCollision;

        // Write to buffer
        TDArray<u8> buffer(totalSize);
        buffer.Resize(totalSize);
        memset(buffer.Data(), 0, totalSize);

        u8* ptr = buffer.Data();

        // Header
        memcpy(ptr, &header, sizeof(LMeshHeader));
        ptr = buffer.Data() + sizeof(LMeshHeader);

        // Descriptors
        for(int i = 0; i < descriptors.Size(); i++)
        {
            memcpy(ptr, &descriptors[i], sizeof(LMeshDescriptor));
            ptr += sizeof(LMeshDescriptor);
        }

        // Material data
        for(int i = 0; i < data.Materials.Size(); i++)
        {
            auto& mat = data.Materials[i];

            // Name length + name
            u32 nameLen = (u32)mat.Name.size();
            memcpy(ptr, &nameLen, 4); ptr += 4;
            memcpy(ptr, mat.Name.c_str(), nameLen + 1); ptr += nameLen + 1;

            // Properties
            memcpy(ptr, &mat.Properties, sizeof(Graphics::MaterialProperties));
            ptr += sizeof(Graphics::MaterialProperties);

            // Material render flags
            memcpy(ptr, &mat.Flags, 4); ptr += 4;

            // Texture count
            u32 texCount = 0;
            for(u32 s = 0; s < LMESH_MAX_TEXTURE_SLOTS; s++)
                if(!mat.TexturePaths[s].empty()) texCount++;

            memcpy(ptr, &texCount, 4); ptr += 4;

            for(u32 s = 0; s < LMESH_MAX_TEXTURE_SLOTS; s++)
            {
                if(!mat.TexturePaths[s].empty())
                {
                    memcpy(ptr, &s, 4); ptr += 4;
                    u8 wrap   = (u8)mat.TextureWraps[s];
                    u8 filter = (u8)mat.TextureFilters[s];
                    *ptr++ = wrap;
                    *ptr++ = filter;
                    u32 pathLen = (u32)mat.TexturePaths[s].size();
                    memcpy(ptr, &pathLen, 4); ptr += 4;
                    memcpy(ptr, mat.TexturePaths[s].c_str(), pathLen + 1); ptr += pathLen + 1;
                }
            }
        }

        // Vertex/index data
        for(int i = 0; i < data.Meshes.Size(); i++)
        {
            auto& mesh = data.Meshes[i];
            u8* vPtr = buffer.Data() + descriptors[i].VertexOffset;

            if(mesh.Animated)
                memcpy(vPtr, mesh.AnimVertices.Data(), descriptors[i].VertexSize);
            else
                memcpy(vPtr, mesh.Vertices.Data(), descriptors[i].VertexSize);

            u8* iPtr = buffer.Data() + descriptors[i].IndexOffset;
            memcpy(iPtr, mesh.Indices.Data(), descriptors[i].IndexSize);
        }

        // Bounding boxes
        u8* bPtr = buffer.Data() + boundsOffset;
        for(int i = 0; i < data.Meshes.Size(); i++)
        {
            auto& mesh = data.Meshes[i];
            Maths::BoundingBox box;
            u32 vertCount = mesh.Animated ? (u32)mesh.AnimVertices.Size() : (u32)mesh.Vertices.Size();

            for(u32 v = 0; v < vertCount; v++)
            {
                Vec3 pos = mesh.Animated ? mesh.AnimVertices[v].Position : mesh.Vertices[v].Position;
                box.Merge(pos);
            }

            LMeshBounds bounds;
            bounds.Min = box.Min();
            bounds.Max = box.Max();
            memcpy(bPtr, &bounds, sizeof(LMeshBounds));
            bPtr += sizeof(LMeshBounds);
        }

        // Collision mesh
        if(hasCollision)
        {
            u8* cPtr = buffer.Data() + collisionOffset;
            LMeshCollisionHeader colHeader;
            colHeader.VertexCount = (u32)collisionVerts.Size();
            colHeader.IndexCount  = (u32)collisionIndices.Size();
            memcpy(cPtr, &colHeader, sizeof(LMeshCollisionHeader));
            cPtr += sizeof(LMeshCollisionHeader);

            memcpy(cPtr, collisionVerts.Data(), collisionVerts.Size() * sizeof(Vec3));
            cPtr += collisionVerts.Size() * sizeof(Vec3);

            memcpy(cPtr, collisionIndices.Data(), collisionIndices.Size() * sizeof(u32));
        }

        // Write file
        return FileSystem::WriteFile(outputPath, buffer.Data(), totalSize);
    }
}
