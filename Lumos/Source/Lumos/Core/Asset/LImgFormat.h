#pragma once
#include "Core/Types.h"

namespace Lumos
{
    static constexpr u32 LIMG_VERSION = 1;
    static constexpr u8 LIMG_MAGIC[4] = { 'L', 'I', 'M', 'G' };

    enum LImgFlags : u32
    {
        LImgFlag_None       = 0,
        LImgFlag_Compressed = BIT(0),
        LImgFlag_HDR        = BIT(1),
    };

    // 32 bytes
    struct LImgHeader
    {
        u8  Magic[4];
        u32 Version;
        u32 Flags;
        u32 Width;
        u32 Height;
        u32 Format;           // RHIFormat value
        u32 CompressedSize;   // size after LZ4 compression
        u32 UncompressedSize; // W * H * BPP
    };
}
