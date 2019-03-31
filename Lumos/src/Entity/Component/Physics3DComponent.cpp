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
		m_Entity->GetTransform()->SetWorldMatrix(m_PhysicsObject->GetWorldSpaceTransform() * m_Entity->GetTransform()->m_Transform.GetLocalMatrix());
	}

	void Physics3DComponent::OnUpdateTransform(const maths::Matrix4& entityTransform)
	{
		m_PhysicsObject->SetPosition(entityTransform.GetPositionVector());
		m_PhysicsObject->SetOrientation(maths::Quaternion(maths::Matrix4::GetEulerAngles(entityTransform.GetRotation()), 1.0f));
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

            ImGui::DragFloat3("Position", &pos.x);
			m_PhysicsObject->SetPosition(pos);

			ImGui::DragFloat3("Velocity", &velocity.x);
			m_PhysicsObject->SetLinearVelocity(velocity);

			ImGui::DragFloat3("Torque", &torque.x);
			m_PhysicsObject->SetTorque(torque);

			ImGui::DragFloat3("Orientation", &orientation.x);
			m_PhysicsObject->SetOrientation(orientation);

			ImGui::DragFloat("Friction", &friction);
			m_PhysicsObject->SetFriction(friction);

			ImGui::DragFloat("Mass", &mass);
			m_PhysicsObject->SetInverseMass(1.0f / mass);

			ImGui::DragFloat("Elasticity", &elasticity);
			m_PhysicsObject->SetElasticity(elasticity);

			ImGui::Checkbox("Static", &isStatic);
			m_PhysicsObject->SetIsStatic(isStatic);

			ImGui::Checkbox("At Rest", &isRest);
			m_PhysicsObject->SetIsAtRest(isRest);

            ImGui::TreePop();
        }
    }
}
