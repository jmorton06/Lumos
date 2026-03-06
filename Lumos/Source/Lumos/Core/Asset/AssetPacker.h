#pragma once
#include "Core/Types.h"
#include "Core/String.h"
#include "Core/DataStructures/TDArray.h"

namespace Lumos
{
    static constexpr u8 LPAK_MAGIC[4] = { 'L', 'P', 'A', 'K' };
    static constexpr u32 LPAK_VERSION = 1;

    struct LPakHeader
    {
        u8  Magic[4];
        u32 Version;
        u32 EntryCount;
        u32 TOCOffset;
        u64 TotalSize;
        u32 Flags;
        u32 Reserved;
    };

    struct LPakTOCEntry
    {
        u64 UUID;
        u32 AssetType;
        u64 DataOffset;
        u64 DataSize;
        u64 UncompressedSize;
        u64 PathHash;
        // Followed by null-terminated path string in TOC
    };

    struct PackSettings
    {
        u64 MaxPackFileSize = 0; // 0 = single file
        TDArray<String8> IncludePaths;
        TDArray<String8> ExcludePaths;
    };

    class AssetPacker
    {
    public:
        static bool Pack(const String8& outputPath, const String8& assetsDir, const PackSettings& settings);
    };
}
