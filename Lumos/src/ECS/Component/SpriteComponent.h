#pragma once
#include "lmpch.h"
#include "LumosComponent.h"
#include "Graphics/Sprite.h"

namespace Lumos
{
	class LUMOS_EXPORT SpriteComponent : public LumosComponent
	{
	public:
        SpriteComponent();
		explicit SpriteComponent(Ref<Graphics::Sprite>& sprite);

		void OnImGui() override;
        
        Graphics::Sprite* GetSprite() const { return m_Sprite.get(); }

		nlohmann::json Serialise() override { return nullptr; };
		void Deserialise(nlohmann::json& data) override {};
        
    private:
        Ref<Graphics::Sprite> m_Sprite;
	};
}
