#pragma once
#include "lmpch.h"
#include "Graphics/Sprite.h"

namespace Lumos
{
	class LUMOS_EXPORT SpriteComponent
	{
	public:
        SpriteComponent();
		explicit SpriteComponent(Ref<Graphics::Sprite>& sprite);

		void OnImGui();
        
        Graphics::Sprite* GetSprite() const { return m_Sprite.get(); }

    private:
        Ref<Graphics::Sprite> m_Sprite;
	};
}
