#pragma once

namespace Lumos
{
    namespace Graphics
    {
        class Mesh;

    }
    struct MeshAsset
    {
        const char* name;
        u64 ID;
        u64 materialID;

        u32* Indices;
        u32 IndexCount;

        u8* VertedData; // Can be cast to Vertex or AnimVertex;
        u32 VertexDataSize;

        bool Animated;
    };

    void Serialise(const char* path, MeshAsset& asset);
    Graphics::Mesh* Deserialise(const char* path);
}