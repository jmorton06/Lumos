#pragma once
#include "LM.h"
#include "JMComponent.h"

namespace Lumos
{
	class PhysicsObject2D;

	class LUMOS_EXPORT Physics2DComponent : public JMComponent
	{
	public:
		std::shared_ptr<PhysicsObject2D> m_PhysicsObject;
	public:
		explicit Physics2DComponent(std::shared_ptr<PhysicsObject2D>& physics);

		static ComponentType GetStaticType()
		{
			static ComponentType type(ComponentType::Physics2D);
			return type;
		}

		inline virtual ComponentType GetType() const override { return GetStaticType(); }
	};
}
