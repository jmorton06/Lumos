#pragma once
#include "LM.h"
#include "LumosComponent.h"

namespace lumos
{
	namespace graphics
	{
		class Sprite;
	}

	class LUMOS_EXPORT SpriteComponent : public LumosComponent
	{
	public:
		std::shared_ptr<graphics::Sprite> m_Sprite;
	public:
		explicit SpriteComponent(std::shared_ptr<graphics::Sprite>& sprite);

		static ComponentType GetStaticType()
		{
			static ComponentType type(ComponentType::Sprite);
			return type;
		}

		void OnUpdateComponent(float dt) override;

		inline virtual ComponentType GetType() const override { return GetStaticType(); }

		void OnIMGUI() override;
	};
}
