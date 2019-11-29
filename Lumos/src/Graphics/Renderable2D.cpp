#include "lmpch.h"
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

		const std::vector<Maths::Vector2>& Renderable2D::GetDefaultUVs()
		{
			static std::vector<Maths::Vector2> results;
			if (results.empty())
			{
				results.push_back(Maths::Vector2(0, 1));
				results.push_back(Maths::Vector2(1, 1));
				results.push_back(Maths::Vector2(1, 0));
				results.push_back(Maths::Vector2(0, 0));
			}
			return results;
		}
	}
}
