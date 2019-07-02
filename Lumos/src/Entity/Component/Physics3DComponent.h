#pragma once
#include "LM.h"
#include "LumosComponent.h"

namespace Lumos
{
	class PhysicsObject3D;

	class LUMOS_EXPORT Physics3DComponent : public LumosComponent
	{
	public:
		std::shared_ptr<PhysicsObject3D> m_PhysicsObject;
	public:
		explicit Physics3DComponent(std::shared_ptr<PhysicsObject3D>& physics);

		void Init() override;
		void OnUpdateComponent(float dt) override;
		void OnUpdateTransform(const Maths::Matrix4& entityTransform) override;
		void DebugDraw(uint64 debugFlags) override;
        
        void OnIMGUI() override;
	};
}
