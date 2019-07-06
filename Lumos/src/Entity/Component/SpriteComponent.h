#pragma once
#include "LM.h"
#include "LumosComponent.h"

namespace Lumos
{
	namespace Graphics
	{
		class Sprite;
	}

	class LUMOS_EXPORT SpriteComponent : public LumosComponent
	{
	public:
        SpriteComponent();
		explicit SpriteComponent(std::shared_ptr<Graphics::Sprite>& sprite);

		void OnUpdateComponent(float dt) override;

		void OnIMGUI() override;
        
        Graphics::Sprite* GetSprite() const { m_Sprite.get(); }
        
    private:
        std::shared_ptr<Graphics::Sprite> m_Sprite;
	};
}
