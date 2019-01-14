#pragma once
#include "JM.h"
#include "JMComponent.h"

namespace jm
{
	class Sprite;

	class JM_EXPORT SpriteComponent : public JMComponent
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
