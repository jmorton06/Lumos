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

		static ComponentType GetStaticType()
		{
			static ComponentType type(ComponentType::Physics3D);
			return type;
		}

		void Init() override;
		void OnUpdateComponent(float dt) override;
		void OnUpdateTransform(const maths::Matrix4& entityTransform) override;
		void DebugDraw(uint64 debugFlags) override;

		inline virtual ComponentType GetType() const override { return GetStaticType(); }
        
        void OnIMGUI() override;
	};
}
