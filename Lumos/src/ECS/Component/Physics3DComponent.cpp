#include "lmpch.h"
#include "Physics3DComponent.h"
#include "App/Application.h"
#include "Physics/LumosPhysicsEngine/LumosPhysicsEngine.h"
#include "ECS/EntityManager.h"

#include <imgui/imgui.h>

namespace Lumos
{
    Physics3DComponent::Physics3DComponent()
    {
        m_PhysicsObject = CreateRef<PhysicsObject3D>();
    }
    
	Physics3DComponent::Physics3DComponent(Ref<PhysicsObject3D>& physics)
	{
        m_PhysicsObject = physics;
	}

	void Physics3DComponent::Init()
	{
	}

	void Physics3DComponent::Update()
	{
	}

    void Physics3DComponent::OnImGui()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2,2));
        ImGui::Columns(2);
        ImGui::Separator();
            
        auto pos = m_PhysicsObject->GetPosition();
        auto force = m_PhysicsObject->GetForce();
        auto torque = m_PhysicsObject->GetTorque();
        auto orientation = m_PhysicsObject->GetOrientation();
		auto angularVelocity = m_PhysicsObject->GetAngularVelocity();
        auto friction = m_PhysicsObject->GetFriction();
        auto isStatic = m_PhysicsObject->GetIsStatic();
        auto isRest = m_PhysicsObject->GetIsAtRest();
        auto mass = 1.0f / m_PhysicsObject->GetInverseMass();
        auto velocity = m_PhysicsObject->GetLinearVelocity();
        auto elasticity = m_PhysicsObject->GetElasticity();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Position");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat3("##Position", Maths::ValuePointer(pos)))
            m_PhysicsObject->SetPosition(pos);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Velocity");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat3("##Velocity", Maths::ValuePointer(velocity)))
            m_PhysicsObject->SetLinearVelocity(velocity);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Torque");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat3("##Torque", Maths::ValuePointer(torque)))
            m_PhysicsObject->SetTorque(torque);
               
        ImGui::PopItemWidth();
        ImGui::NextColumn();
	
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Orientation");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat4("##Orientation", Maths::ValuePointer(orientation)))
            m_PhysicsObject->SetOrientation(orientation);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Angular Velocity");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		if (ImGui::DragFloat3("##Angular Velocity", Maths::ValuePointer(angularVelocity)))
			m_PhysicsObject->SetAngularVelocity(angularVelocity);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
            
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Friction");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat("##Friction", &friction))
            m_PhysicsObject->SetFriction(friction);

        ImGui::PopItemWidth();
        ImGui::NextColumn();
			
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Mass");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat("##Mass", &mass))
            m_PhysicsObject->SetInverseMass(1.0f / mass);
            
        ImGui::PopItemWidth();
        ImGui::NextColumn();
		
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Elasticity");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat("##Elasticity", &elasticity))
            m_PhysicsObject->SetElasticity(elasticity);
            
        ImGui::PopItemWidth();
        ImGui::NextColumn();
		
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Static");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::Checkbox("##Static", &isStatic))
            m_PhysicsObject->SetIsStatic(isStatic);

        ImGui::PopItemWidth();
        ImGui::NextColumn();
			

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("At Rest");
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
