#pragma once
#include "LM.h"
#include "JMComponent.h"

namespace Lumos
{
	class PhysicsObject3D;

	class LUMOS_EXPORT Physics3DComponent : public JMComponent
	{
	public:
		std::shared_ptr<PhysicsObject3D> m_PhysicsObject;
	public:
		explicit Physics3DComponent(std::shared_ptr<PhysicsObject3D>& physics);

		static ComponentType GetStaticType()
		{
			static ComponentType type(ComponentType::Physics3D);
			return type;
		}

		void Init() override;
		void OnUpdateComponent(float dt) override;
		void DebugDraw(uint64 debugFlags) override;

		inline virtual ComponentType GetType() const override { return GetStaticType(); }
	};
}
