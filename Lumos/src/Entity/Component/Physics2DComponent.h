#pragma once
#include "LM.h"
#include "LumosComponent.h"

namespace Lumos
{
	class PhysicsObject2D;

	class LUMOS_EXPORT Physics2DComponent : public LumosComponent
	{
	public:
		std::shared_ptr<PhysicsObject2D> m_PhysicsObject;
	public:
		explicit Physics2DComponent(std::shared_ptr<PhysicsObject2D>& physics);

		void OnUpdateComponent(float dt) override;
		void OnIMGUI() override;
	};
}
