#ifndef LUMOS_PLATFORM_MACOS
#include "Precompiled.h"
#endif
#include "Renderable2D.h"
#include <glm/ext/vector_float2.hpp>

namespace Lumos
{
    namespace Graphics
    {
        Renderable2D::Renderable2D()
        {
        }

        Renderable2D::~Renderable2D()
        {
        }

        const std::array<glm::vec2, 4>& Renderable2D::GetDefaultUVs()
        {
            LUMOS_PROFILE_FUNCTION();
            static std::array<glm::vec2, 4> results;
            {
                results[0] = glm::vec2(0, 1);
                results[1] = glm::vec2(1, 1);
                results[2] = glm::vec2(1, 0);
                results[3] = glm::vec2(0, 0);
            }
            return results;
        }

        const std::array<glm::vec2, 4>& Renderable2D::GetUVs(const glm::vec2& min, const glm::vec2& max)
        {
            LUMOS_PROFILE_FUNCTION();
            static std::array<glm::vec2, 4> results;
            {
                results[0] = glm::vec2(min.x, max.y);
                results[1] = max;
                results[2] = glm::vec2(max.x, min.y);
                results[3] = min;
            }
            return results;
        }
    }
}
