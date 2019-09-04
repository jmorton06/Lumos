#include "LM.h"
#include "Physics3DComponent.h"
#include "App/Application.h"
#include "Physics/LumosPhysicsEngine/LumosPhysicsEngine.h"
#include "ECS/EntityManager.h"
#include "TransformComponent.h"

#include <imgui/imgui.h>

namespace Lumos
{
    Physics3DComponent::Physics3DComponent()
    {
        m_Name = "Physics3D";
        m_PhysicsObject = CreateRef<PhysicsObject3D>();
    }
    
	Physics3DComponent::Physics3DComponent(Ref<PhysicsObject3D>& physics)
	{
		m_Name = "Physics3D";
        m_PhysicsObject = physics;
	}

	void Physics3DComponent::Init()
	{
		m_PhysicsObject->SetEntity(m_Entity);
	}

	void Physics3DComponent::Update()
	{
        m_Entity->GetTransformComponent()->GetTransform().SetLocalPosition(m_PhysicsObject->GetPosition());
        m_Entity->GetTransformComponent()->GetTransform().SetLocalOrientation(m_PhysicsObject->GetOrientation());
		m_Entity->GetTransformComponent()->GetTransform().UpdateMatrices();
	}

	void Physics3DComponent::OnUpdateTransform(const Maths::Matrix4& entityTransform)
	{
		//m_PhysicsObject->SetPosition(entityTransform.GetPositionVector());
        //m_PhysicsObject->SetOrientation(Maths::Quaternion(Maths::Matrix4::GetEulerAngles(entityTransform.GetRotation()), 1.0f));
	}
    
    void Physics3DComponent::OnIMGUI()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2,2));
        ImGui::Columns(2);
        ImGui::Separator();
            
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

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Position");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat3("##Position", &pos.x))
            m_PhysicsObject->SetPosition(pos);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Velocity");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat3("##Velocity", &velocity.x))
            m_PhysicsObject->SetLinearVelocity(velocity);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::Text("Torque");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat3("##Torque", &torque.x))
            m_PhysicsObject->SetTorque(torque);
               
        ImGui::PopItemWidth();
        ImGui::NextColumn();
	
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Orientation");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat3("##Orientation", &orientation.x))
            m_PhysicsObject->SetOrientation(orientation);

        ImGui::PopItemWidth();
        ImGui::NextColumn();
            
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Friction");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat("##Friction", &friction))
            m_PhysicsObject->SetFriction(friction);

        ImGui::PopItemWidth();
        ImGui::NextColumn();
			
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Mass");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat("##Mass", &mass))
            m_PhysicsObject->SetInverseMass(1.0f / mass);
            
        ImGui::PopItemWidth();
        ImGui::NextColumn();
		
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Elasticity");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat("##Elasticity", &elasticity))
            m_PhysicsObject->SetElasticity(elasticity);
            
        ImGui::PopItemWidth();
        ImGui::NextColumn();
		
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Static");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::Checkbox("##Static", &isStatic))
            m_PhysicsObject->SetIsStatic(isStatic);

        ImGui::PopItemWidth();
        ImGui::NextColumn();
			

        ImGui::AlignTextToFramePadding();
        ImGui::Text("At Rest");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::Checkbox("##At Rest", &isRest))
            m_PhysicsObject->SetIsAtRest(isRest);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::Columns(1);
        ImGui::Separator();
        ImGui::PopStyleVar();
    }
}
