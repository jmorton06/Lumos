#include "Precompiled.h"
#include "MeshAsset.h"
#include "LMeshWriter.h"
#include "LMeshReader.h"
#include "AssetImporter.h"
#include "Core/OS/FileSystem.h"
#include "Graphics/Mesh.h"
#include "Graphics/Model.h"

namespace Lumos
{
    void Serialise(const char* path, MeshAsset& asset)
    {
        if(!path || !asset.Indices || !asset.VertedData)
            return;

        LMeshWriteData data;
        LMeshWriteData::MeshEntry entry;
        entry.Name      = asset.name ? asset.name : "mesh";
        entry.Animated   = asset.Animated;

        if(asset.Animated)
        {
            u32 vertCount = asset.VertexDataSize / sizeof(Graphics::AnimVertex);
            entry.AnimVertices.Resize(vertCount);
            memcpy(entry.AnimVertices.Data(), asset.VertedData, asset.VertexDataSize);
        }
        else
        {
            u32 vertCount = asset.VertexDataSize / sizeof(Graphics::Vertex);
            entry.Vertices.Resize(vertCount);
            memcpy(entry.Vertices.Data(), asset.VertedData, asset.VertexDataSize);
        }

        entry.Indices.Resize(asset.IndexCount);
        memcpy(entry.Indices.Data(), asset.Indices, asset.IndexCount * sizeof(u32));

        data.Meshes.PushBack(std::move(entry));
        data.AssetUUID = asset.ID;

        ImportSettings settings;
        std::string pathStr(path);
        LMeshWriter::Write(Str8StdS(pathStr), data, settings);
    }

    Graphics::Mesh* Deserialise(const char* path)
    {
        if(!path)
            return nullptr;

        Arena* tempArena = ArenaAlloc(Kilobytes(4));

        Graphics::Model model;
        std::string pathStr(path);
        if(!LMeshReader::Read(tempArena, Str8StdS(pathStr), model))
        {
            ArenaRelease(tempArena);
            return nullptr;
        }

        Graphics::Mesh* result = nullptr;
        if(!model.GetMeshes().Empty())
        {
            // Return first mesh, caller takes ownership via SharedPtr elsewhere
            result = new Graphics::Mesh(*model.GetMeshes()[0].get());
        }

        ArenaRelease(tempArena);
        return result;
    }
}
