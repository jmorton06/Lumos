#include "Precompiled.h"
#include "Colour.h"
#include "Maths/Random.h"
#include <cmath>

namespace Lumos
{
    namespace Colour
    {
        Vec4 RandomColour()
        {
            // Generating random RGB values
            float r = Random32::Rand(0.0f, 1.0f);
            float g = Random32::Rand(0.0f, 1.0f);
            float b = Random32::Rand(0.0f, 1.0f);

            return Vec4(r, g, b, 1.0f); // Assuming alpha value of 1.0 (fully opaque)
        }

        // Tanner Helland's blackbody approximation. Temperature in Kelvin.
        Vec4 BlackbodyColour(float kelvin)
        {
            float t = kelvin;
            if(t < 1000.0f)
                t = 1000.0f;
            if(t > 40000.0f)
                t = 40000.0f;
            t /= 100.0f;

            float r, g, b;

            if(t <= 66.0f)
            {
                r = 255.0f;
                g = 99.4708025861f * std::log(t) - 161.1195681661f;
            }
            else
            {
                r = 329.698727446f * std::pow(t - 60.0f, -0.1332047592f);
                g = 288.1221695283f * std::pow(t - 60.0f, -0.0755148492f);
            }

            if(t >= 66.0f)
                b = 255.0f;
            else if(t <= 19.0f)
                b = 0.0f;
            else
                b = 138.5177312231f * std::log(t - 10.0f) - 305.0447927307f;

            auto clamp01 = [](float v)
            { return v < 0.0f ? 0.0f : (v > 255.0f ? 255.0f : v) / 255.0f; };

            return Vec4(clamp01(r), clamp01(g), clamp01(b), 1.0f);
        }
    }
}
