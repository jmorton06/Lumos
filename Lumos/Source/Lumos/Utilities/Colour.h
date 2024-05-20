#pragma once

#include <glm/ext/vector_float4.hpp>

namespace Lumos
{
    namespace Colour
    {
        glm::vec4 RandomColour();

        // List of common colors
        static glm::vec4 Red     = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
        static glm::vec4 Green   = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
        static glm::vec4 Blue    = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
        static glm::vec4 Yellow  = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
        static glm::vec4 Cyan    = glm::vec4(0.0f, 1.0f, 1.0f, 1.0f);
        static glm::vec4 Magenta = glm::vec4(1.0f, 0.0f, 1.0f, 1.0f);
        static glm::vec4 White   = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        static glm::vec4 Black   = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        static glm::vec4 Gray    = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
        static glm::vec4 Orange  = glm::vec4(1.0f, 0.5f, 0.0f, 1.0f);
        static glm::vec4 Purple  = glm::vec4(0.5f, 0.0f, 0.5f, 1.0f);
        static glm::vec4 Brown   = glm::vec4(0.6f, 0.3f, 0.1f, 1.0f);

    }
}
