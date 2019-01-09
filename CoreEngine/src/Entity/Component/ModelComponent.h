#pragma once
#include "JM.h"
#include "JMComponent.h"

namespace jm
{
	class Model;

	class JM_EXPORT ModelComponent : public JMComponent
	{
	public:
		std::shared_ptr<Model> m_Model;
	public:
		explicit ModelComponent(std::shared_ptr<Model>& model);

		static ComponentType GetStaticType()
		{
			static ComponentType type(ComponentType::Model);
			return type;
		}

		void OnUpdateComponent(float dt) override;

		inline virtual ComponentType GetType() const override { return GetStaticType(); }
	};
}
