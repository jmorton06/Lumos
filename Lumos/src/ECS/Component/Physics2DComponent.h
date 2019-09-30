#pragma once
#include "lmpch.h"
#include "Physics/B2PhysicsEngine/PhysicsObject2D.h"

#include <jsonhpp/json.hpp>

namespace Lumos
{
	class LUMOS_EXPORT Physics2DComponent
	{
	public:
        Physics2DComponent();
		explicit Physics2DComponent(Ref<PhysicsObject2D>& physics);

		void Update();
		void OnImGui();
        
        Ref<PhysicsObject2D> GetPhysicsObject() { return m_PhysicsObject; }
		nlohmann::json Serialise() { return nullptr; };
		void Deserialise(nlohmann::json& data) {};
        
    private:
        Ref<PhysicsObject2D> m_PhysicsObject;
	};
}
