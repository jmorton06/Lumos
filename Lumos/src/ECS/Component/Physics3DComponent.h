#pragma once
#include "LM.h"
#include "LumosComponent.h"
#include "Physics/LumosPhysicsEngine/PhysicsObject3D.h"

namespace Lumos
{
	class LUMOS_EXPORT Physics3DComponent : public LumosComponent
	{
	public:
        Physics3DComponent();
		explicit Physics3DComponent(Ref<PhysicsObject3D>& physics);

		~Physics3DComponent() = default;

		void Init();
		void Update();

		void OnUpdateTransform(const Maths::Matrix4& entityTransform) override;
        
        void OnIMGUI() override;
        
        PhysicsObject3D* GetPhysicsObject() const { return m_PhysicsObject.get(); }

		nlohmann::json Serialise() override { return nullptr; };
		void Deserialise(nlohmann::json& data) override {};
    private:
        Ref<PhysicsObject3D> m_PhysicsObject;
	};
}
