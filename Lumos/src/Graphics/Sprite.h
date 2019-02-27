#pragma once
#include "LM.h"
#include "Maths/Vector2.h"
#include "Renderable2D.h"

namespace Lumos
{
	class Texture2D;

	class LUMOS_EXPORT Sprite : public Renderable2D
	{
	public:
		Sprite(const maths::Vector2& position, const maths::Vector2& scale, uint colour);
		Sprite(std::shared_ptr<Texture2D> texture, const maths::Vector2& position, const maths::Vector2& scale, uint colour);
		virtual ~Sprite();
	};
}
