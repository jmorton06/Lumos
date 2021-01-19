#include "Precompiled.h"
#include "Renderable2D.h"
#include "Maths/Maths.h"

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

        const std::array<Maths::Vector2, 4>& Renderable2D::GetDefaultUVs()
		{
            LUMOS_PROFILE_FUNCTION();
			static std::array<Maths::Vector2, 4> results;
			{
				results[0] = Maths::Vector2(0, 1);
				results[1] = Maths::Vector2(1, 1);
				results[2] = Maths::Vector2(1, 0);
				results[3] = Maths::Vector2(0, 0);
			}
			return results;
		}
    
        const std::array<Maths::Vector2, 4>& Renderable2D::GetUVs(const Maths::Vector2& min, const Maths::Vector2& max)
        {
            LUMOS_PROFILE_FUNCTION();
            static std::array<Maths::Vector2, 4> results;
            {
                results[0] = Maths::Vector2(min.x, max.y);
                results[1] = max;
                results[2] = Maths::Vector2(max.x, min.y);
                results[3] = min;
            }
            return results;
        }
	}
}
