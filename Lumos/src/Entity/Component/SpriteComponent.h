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
		std::shared_ptr<Graphics::Sprite> m_Sprite;
	public:
		explicit SpriteComponent(std::shared_ptr<Graphics::Sprite>& sprite);

		void OnUpdateComponent(float dt) override;

		void OnIMGUI() override;
	};
}
