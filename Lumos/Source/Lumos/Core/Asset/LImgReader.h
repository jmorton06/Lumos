#pragma once
#include "Core/Types.h"
#include "Graphics/RHI/RHIDefinitions.h"

namespace Lumos
{
    struct String8;

    struct LImgReadResult
    {
        u8*  Pixels;
        u32  Width;
        u32  Height;
        u32  Bits; // bits per pixel (32 for RGBA8, 128 for RGBA32F)
        bool IsHDR;
    };

    class LImgReader
    {
    public:
        static bool Read(const char* physicalPath, LImgReadResult& outResult);
        // same, from bytes already in memory (a packed build has no file to open)
        static bool ReadMemory(const u8* data, u64 size, LImgReadResult& outResult);
    };
}
