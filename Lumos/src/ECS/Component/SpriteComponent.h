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
		explicit SpriteComponent(Ref<Graphics::Sprite>& sprite);

		void OnUpdateComponent(float dt) override;

		void OnIMGUI() override;
        
        Graphics::Sprite* GetSprite() const { return m_Sprite.get(); }

		nlohmann::json Serialise() override { return nullptr; };
		void Deserialise(nlohmann::json& data) override {};
        
    private:
        Ref<Graphics::Sprite> m_Sprite;
	};
}
