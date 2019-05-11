#include "LM.h"
#include "Physics2DComponent.h"
#include "Physics/B2PhysicsEngine/PhysicsObject2D.h"
#include "Maths/MathsUtilities.h"

#include <imgui/imgui.h>
#include "Entity/Entity.h"

#include <math.h>

namespace Lumos
{
	Physics2DComponent::Physics2DComponent(std::shared_ptr<PhysicsObject2D>& physics)
		: m_PhysicsObject(physics)
	{

	}

	void Physics2DComponent::OnUpdateComponent(float dt)
	{
        auto angle = m_PhysicsObject->GetAngle();
        auto qw = cos(angle/2);
        auto qz = 1.0f * sin(angle/2);
        
        m_Entity->GetTransform()->m_Transform.SetLocalPosition(maths::Vector3(m_PhysicsObject->GetPosition(), 1.0f));
        m_Entity->GetTransform()->m_Transform.SetLocalOrientation(maths::Quaternion(0.0f, 0.0f, qz, qw));
		m_Entity->GetTransform()->m_Transform.UpdateMatrices();
	}

	void Physics2DComponent::OnIMGUI()
	{
		if (ImGui::TreeNode("Physics2D"))
		{
			auto pos = m_PhysicsObject->GetPosition();
			auto angle = m_PhysicsObject->GetAngle();
			auto friction = m_PhysicsObject->GetFriction();
			auto isStatic = m_PhysicsObject->GetIsStatic();
			auto isRest = m_PhysicsObject->GetIsAtRest();

			auto elasticity = m_PhysicsObject->GetElasticity();

			ImGui::DragFloat2("Position", &pos.x);
			m_PhysicsObject->SetPosition(pos);

			ImGui::DragFloat("Orientation", &angle);

			ImGui::DragFloat("Friction", &friction);
			m_PhysicsObject->SetFriction(friction);

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
