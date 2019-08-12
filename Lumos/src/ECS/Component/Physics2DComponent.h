#pragma once
#include "LM.h"
#include "LumosComponent.h"

namespace Lumos
{
	class PhysicsObject2D;

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
