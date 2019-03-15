#include "LM.h"
#include "Physics2DComponent.h"
#include "Physics/B2PhysicsEngine/PhysicsObject2D.h"

#include <imgui/imgui.h>
#include "Entity/Entity.h"
#include "Sandbox/Scenes/SceneLuaTest.h"

namespace Lumos
{
	Physics2DComponent::Physics2DComponent(std::shared_ptr<PhysicsObject2D>& physics)
		: m_PhysicsObject(physics)
	{

	}

	void Physics2DComponent::OnUpdateComponent(float dt)
	{
		//m_Entity->GetTransform()->m_Transform.SetOrientation(maths::Quaternion::EulerAnglesToQuaternion(0.0f, 0.0f, -m_PhysicsObject->GetAngle()));
		//m_Entity->GetTransform()->m_Transform.SetWorldPosition(maths::Vector3(m_PhysicsObject->GetPosition(),1.0f));

		auto transform = maths::Matrix4::Translation(maths::Vector3(m_PhysicsObject->GetPosition(), 1.0f)) * maths::Matrix4::RotationZ(maths::RadToDeg(m_PhysicsObject->GetAngle()));

		m_Entity->GetTransform()->SetWorldMatrix(transform * m_Entity->GetTransform()->m_Transform.GetLocalMatrix());

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
