#pragma once
#include "JM.h"

class Sprite;

namespace jm
{

	class Renderer2D
	{
	public:
		Renderer2D();
		~Renderer2D();

	private:
		std::vector<Sprite*> m_Sprites;
		uint m_ScreenBufferWidth, m_ScreenBufferHeight;

	};
}
