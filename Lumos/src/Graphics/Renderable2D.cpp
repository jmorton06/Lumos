#include "LM.h"
#include "Renderable2D.h"

namespace Lumos
{
	Renderable2D::Renderable2D()
	{
	}

	Renderable2D::~Renderable2D()
	{
		
	}

	const std::vector<maths::Vector2>& Renderable2D::GetDefaultUVs()
	{
		static std::vector<maths::Vector2> results;
		if (!results.size())
		{
			results.push_back(maths::Vector2(0, 1));
			results.push_back(maths::Vector2(1, 1));
			results.push_back(maths::Vector2(1, 0));
			results.push_back(maths::Vector2(0, 0));
		}
		return results;
	}
}
