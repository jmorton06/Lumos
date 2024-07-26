#include "Precompiled.h"
#include "Renderable2D.h"
#include "Maths/Vector2.h"

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

        const std::array<Vec2, 4>& Renderable2D::GetDefaultUVs()
        {
            LUMOS_PROFILE_FUNCTION();
            static std::array<Vec2, 4> results;
            {
                results[0] = Vec2(0, 1);
                results[1] = Vec2(1, 1);
                results[2] = Vec2(1, 0);
                results[3] = Vec2(0, 0);
            }
            return results;
        }

        const std::array<Vec2, 4>& Renderable2D::GetUVs(const Vec2& min, const Vec2& max)
        {
            LUMOS_PROFILE_FUNCTION();
            static std::array<Vec2, 4> results;
            {
                results[0] = Vec2(min.x, max.y);
                results[1] = max;
                results[2] = Vec2(max.x, min.y);
                results[3] = min;
            }
            return results;
        }
    }
}
