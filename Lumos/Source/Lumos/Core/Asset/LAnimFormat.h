#pragma once
#include "Core/Types.h"

namespace Lumos
{
    static constexpr u32 LANIM_VERSION = 1;
    static constexpr u8 LANIM_MAGIC[4] = { 'L', 'A', 'N', 'M' };

    // 48 bytes
    struct LAnimHeader
    {
        u8  Magic[4];
        u32 Version;
        u64 AssetUUID;
        u32 SkeletonBlobSize;  // 0 if no skeleton
        u32 AnimationCount;
        u32 BindPoseCount;     // number of Mat4s
        u32 TotalFileSize;
        u32 Reserved[2];
    };
}
