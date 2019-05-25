#pragma once
#include "LM.h"
#include "Maths/Vector2.h"
#include "Renderable2D.h"

namespace lumos
{
	namespace maths 
	{
		class Vector4;
	}

	namespace graphics
	{
		class Texture2D;

		class LUMOS_EXPORT Sprite : public Renderable2D
		{
		public:
			Sprite(const maths::Vector2& position, const maths::Vector2& scale, const maths::Vector4& colour);
			Sprite(std::shared_ptr<Texture2D> texture, const maths::Vector2& position, const maths::Vector2& scale, const maths::Vector4& colour);
			virtual ~Sprite();
			void SetPosition(const maths::Vector2& vector2) { m_Position = vector2; };
		};
	}
}
