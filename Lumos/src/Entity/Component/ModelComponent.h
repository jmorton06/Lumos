#pragma once
#include "LM.h"
#include "JMComponent.h"

namespace Lumos
{
	class Model;

	class LUMOS_EXPORT ModelComponent : public JMComponent
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
