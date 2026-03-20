#pragma once
#include <stdint.h>

namespace Lumos
{
    namespace Embedded
    {
        // Splash screen data
        struct SplashData
        {
            const uint32_t width;
            const uint32_t height;
            const uint8_t* data;
        };

        const SplashData& GetSplashData();

        // BRDF LUT data
        struct BRDFData
        {
            const uint32_t width;
            const uint32_t height;
            const uint8_t* data;
        };

        const BRDFData& GetBRDFData();
    }
}
