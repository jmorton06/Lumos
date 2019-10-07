#pragma once
#include "lmpch.h"
#include "Physics/LumosPhysicsEngine/PhysicsObject3D.h"

#include <jsonhpp/json.hpp>

namespace Lumos
{
	class LUMOS_EXPORT Physics3DComponent
	{
	public:
        Physics3DComponent();
		explicit Physics3DComponent(Ref<PhysicsObject3D>& physics);

		~Physics3DComponent() = default;

		void Init();
		void Update();
        void OnImGui();
        
		const Ref<PhysicsObject3D>& GetPhysicsObject() const { return m_PhysicsObject; }

		nlohmann::json Serialise() { return nullptr; };
		void Deserialise(nlohmann::json& data) {};

    private:
        Ref<PhysicsObject3D> m_PhysicsObject;
	};
}
