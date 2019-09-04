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

		void Update();
		void OnIMGUI() override;
        
        Ref<PhysicsObject2D> GetPhysicsObject() { return m_PhysicsObject; }
		nlohmann::json Serialise() override { return nullptr; };
		void Deserialise(nlohmann::json& data) override {};
        
    private:
        Ref<PhysicsObject2D> m_PhysicsObject;
	};
}
