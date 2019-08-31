#pragma once
#include "LM.h"
#include "LumosComponent.h"
#include "Physics/B2PhysicsEngine/PhysicsObject2D.h"

namespace Lumos
{
	class LUMOS_EXPORT Physics2DComponent : public LumosComponent
	{
	public:
        Physics2DComponent();
		explicit Physics2DComponent(Ref<PhysicsObject2D>& physics);

		void OnUpdateComponent(float dt) override;
		void OnIMGUI() override;
        
        PhysicsObject2D* GetPhysicsObject() const { return m_PhysicsObject.get(); }
		nlohmann::json Serialise() override { return nullptr; };
		void Deserialise(nlohmann::json& data) override {};
        
    private:
        Ref<PhysicsObject2D> m_PhysicsObject;
	};
}
