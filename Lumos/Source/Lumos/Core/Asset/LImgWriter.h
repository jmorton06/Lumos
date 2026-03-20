#pragma once
#include "Core/Types.h"
#include "Graphics/RHI/RHIDefinitions.h"

namespace Lumos
{
    struct String8;

    class LImgWriter
    {
    public:
        static bool Write(String8 path, u32 width, u32 height, const u8* pixels, Graphics::RHIFormat format);
    };
}
