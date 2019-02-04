#pragma once
#include "LM.h"

class Sprite;

namespace Lumos
{

	class LUMOS_EXPORT Renderer2D
	{
	public:
		Renderer2D();
		~Renderer2D();

	private:
		std::vector<Sprite*> m_Sprites;
		uint m_ScreenBufferWidth, m_ScreenBufferHeight;

	};
}
