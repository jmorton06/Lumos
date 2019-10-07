#pragma once
#include "lmpch.h"
#include "Maths/Vector2.h"
#include "Renderable2D.h"

namespace Lumos
{
	namespace Maths 
	{
		class Vector4;
	}

	namespace Graphics
	{
		class Texture2D;

		class LUMOS_EXPORT Sprite : public Renderable2D
		{
		public:
			Sprite(const Maths::Vector2& position = Maths::Vector2(0.0f,0.0f), const Maths::Vector2& scale = Maths::Vector2(1.0f,1.0f), const Maths::Vector4& colour = Maths::Vector4(1.0f));
			Sprite(Ref<Texture2D> texture, const Maths::Vector2& position, const Maths::Vector2& scale, const Maths::Vector4& colour);
			virtual ~Sprite();
			void SetPosition(const Maths::Vector2& vector2) { m_Position = vector2; };

			void OnImGui();
		};
	}
}
