#include "LM.h"
#include "Physics3DComponent.h"
#include "Physics/LumosPhysicsEngine/PhysicsObject3D.h"
#include "Physics/LumosPhysicsEngine/LumosPhysicsEngine.h"
#include "Entity/Entity.h"

#include <imgui/imgui.h>

namespace Lumos
{
	Physics3DComponent::Physics3DComponent(std::shared_ptr<PhysicsObject3D>& physics)
		: m_PhysicsObject(physics)
	{
		LumosPhysicsEngine::Instance()->AddPhysicsObject(physics);
	}

	void Physics3DComponent::Init()
	{
		m_PhysicsObject->SetEntity(m_Entity);
	}

	void Physics3DComponent::OnUpdateComponent(float dt)
	{
		m_Entity->SetPosition(m_PhysicsObject->GetPosition());
	}

	void Physics3DComponent::DebugDraw(uint64 debugFlags)
	{
		m_PhysicsObject->DebugDraw(debugFlags);

		if (debugFlags & DEBUGDRAW_FLAGS_COLLISIONVOLUMES)
		{
			if (m_PhysicsObject->GetCollisionShape())
				m_PhysicsObject->GetCollisionShape()->DebugDraw(m_PhysicsObject.get());
		}
	}
    
    void Physics3DComponent::OnIMGUI()
    {
        if (ImGui::TreeNode("Physics3D"))
        {
            auto pos = m_PhysicsObject->GetPosition();
            auto force = m_PhysicsObject->GetForce();
            auto torque = m_PhysicsObject->GetTorque();
            auto orientation = m_PhysicsObject->GetOrientation();
            auto friction = m_PhysicsObject->GetFriction();
            auto isStatic = m_PhysicsObject->GetIsStatic();
            auto isRest = m_PhysicsObject->GetIsAtRest();
            auto mass = 1.0f / m_PhysicsObject->GetInverseMass();
            auto velocity = m_PhysicsObject->GetLinearVelocity();

            auto elasticity = m_PhysicsObject->GetElasticity();

            ImGui::Text("Position    : %5.2f, %5.2f, %5.2f", pos.GetX(), pos.GetY(), pos.GetZ());
            ImGui::Text("Velocity    : %5.2f, %5.2f, %5.2f", velocity.GetX(), velocity.GetY(), velocity.GetZ());
            ImGui::Text("Force       : %5.2f, %5.2f, %5.2f", force.GetX(), force.GetY(), force.GetZ());
            ImGui::Text("Torque      : %5.2f, %5.2f, %5.2f", torque.GetX(), torque.GetY(), torque.GetZ());
            ImGui::Text("Orientation : %5.2f, %5.2f, %5.2f", orientation.x, orientation.y, orientation.z);
            ImGui::Text("Friction    : %5.2f", friction);
            ImGui::Text("Mass        : %5.2f", mass);

            ImGui::Text("Elasticity  : %5.2f", elasticity);
            ImGui::Text("Is Static   : %s", isStatic ? "true" : "false");
            ImGui::Text("At Rest     : %s", isRest ? "true" : "false");

            
            ImGui::TreePop();
        }
    }
}
