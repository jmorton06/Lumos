#pragma once
#include "LM.h"
#include "JMComponent.h"

namespace Lumos
{
	class Sprite;

	class LUMOS_EXPORT SpriteComponent : public JMComponent
	{
	public:
		std::shared_ptr<Sprite> m_Sprite;
	public:
		explicit SpriteComponent(std::shared_ptr<Sprite>& sprite);

		static ComponentType GetStaticType()
		{
			static ComponentType type(ComponentType::Sprite);
			return type;
		}

		void OnUpdateComponent(float dt) override;

		inline virtual ComponentType GetType() const override { return GetStaticType(); }
	};
}
