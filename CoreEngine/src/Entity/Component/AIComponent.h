#pragma once
#include "JM.h"
#include "JMComponent.h"

namespace jm
{
	class AINode;

	class JM_EXPORT AIComponent : public JMComponent
	{
	public:
		std::shared_ptr<AINode> m_AINode;
	public:
		explicit AIComponent(std::shared_ptr<AINode>& aiNode);

		static ComponentType GetStaticType()
		{
			static ComponentType type(ComponentType::AI);
			return type;
		}

		inline virtual ComponentType GetType() const override { return GetStaticType(); }
	};
}
