#pragma once

#include "Maths/Vector4.h"

namespace Lumos
{
    namespace Colour
    {
        Vec4 RandomColour();

        // List of common colors
        static Vec4 Red     = Vec4(1.0f, 0.0f, 0.0f, 1.0f);
        static Vec4 Green   = Vec4(0.0f, 1.0f, 0.0f, 1.0f);
        static Vec4 Blue    = Vec4(0.0f, 0.0f, 1.0f, 1.0f);
        static Vec4 Yellow  = Vec4(1.0f, 1.0f, 0.0f, 1.0f);
        static Vec4 Cyan    = Vec4(0.0f, 1.0f, 1.0f, 1.0f);
        static Vec4 Magenta = Vec4(1.0f, 0.0f, 1.0f, 1.0f);
        static Vec4 White   = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
        static Vec4 Black   = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
        static Vec4 Gray    = Vec4(0.5f, 0.5f, 0.5f, 1.0f);
        static Vec4 Orange  = Vec4(1.0f, 0.5f, 0.0f, 1.0f);
        static Vec4 Purple  = Vec4(0.5f, 0.0f, 0.5f, 1.0f);
        static Vec4 Brown   = Vec4(0.6f, 0.3f, 0.1f, 1.0f);

    }
}
