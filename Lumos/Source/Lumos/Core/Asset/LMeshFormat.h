#pragma once
#include "Core/Types.h"
#include "Maths/Vector3.h"

namespace Lumos
{
    // .lmesh binary format version
    static constexpr u32 LMESH_VERSION = 3;
    static constexpr u8 LMESH_MAGIC[4] = { 'L', 'M', 'S', 'H' };

    enum LMeshFlags : u32
    {
        LMeshFlag_None          = 0,
        LMeshFlag_HasAnimation  = BIT(0),
        LMeshFlag_HasCollision  = BIT(1),
        LMeshFlag_Compressed    = BIT(2),
    };

    enum LMeshVertexFlags : u32
    {
        LMeshVertexFlag_None     = 0,
        LMeshVertexFlag_Animated = BIT(0),
    };

    // 48 bytes
    struct LMeshHeader
    {
        u8  Magic[4];
        u32 Version;
        u32 Flags;
        u64 AssetUUID;
        u32 MeshCount;
        u32 MaterialCount;
        u32 SkeletonOffset;
        u32 AnimationCount;
        u32 TotalFileSize;
        u32 Reserved;
    };

    // 40 bytes per mesh
    struct LMeshDescriptor
    {
        u64 NameHash;
        u32 VertexOffset;
        u32 VertexSize;
        u32 VertexCount;
        u32 IndexOffset;
        u32 IndexSize;
        u32 IndexCount;
        u32 MaterialIndex;
        u32 Flags;
    };

    static constexpr u32 LMESH_MAX_TEXTURE_SLOTS = 6;

    // Variable size - serialized sequentially
    struct LMeshTextureRef
    {
        u32 Slot; // 0=albedo 1=normal 2=metallic 3=roughness 4=ao 5=emissive
        // Followed by null-terminated path string
    };

    // Bounding box stored per mesh after vertex/index data
    struct LMeshBounds
    {
        Vec3 Min;
        Vec3 Max;
    };

    // Collision mesh header
    struct LMeshCollisionHeader
    {
        u32 VertexCount;
        u32 IndexCount;
        // Followed by Vec3[VertexCount] + u32[IndexCount]
    };
}
