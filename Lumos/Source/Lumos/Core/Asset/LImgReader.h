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
    };
}
