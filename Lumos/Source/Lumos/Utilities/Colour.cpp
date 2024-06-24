#ifndef LUMOS_PLATFORM_MACOS
#include "Precompiled.h"
#endif
#include "Colour.h"
#include "Maths/Random.h"

namespace Lumos
{
    namespace Colour
    {
        glm::vec4 RandomColour()
        {
            // Generating random RGB values
            float r = Random32::Rand(0.0f, 1.0f);
            float g = Random32::Rand(0.0f, 1.0f);
            float b = Random32::Rand(0.0f, 1.0f);

            return glm::vec4(r, g, b, 1.0f); // Assuming alpha value of 1.0 (fully opaque)
        }
    }
}
