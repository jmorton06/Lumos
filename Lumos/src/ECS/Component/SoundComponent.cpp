#include "LM.h"
#include "SoundComponent.h"
#include "Audio/SoundNode.h"
#include "App/Scene.h"
#include "Maths/BoundingSphere.h"
#include "App/Application.h"
#include "Audio/AudioManager.h"
#include <imgui/imgui.h>
#include "Physics/LumosPhysicsEngine/PhysicsObject3D.h"

namespace Lumos
{
    SoundComponent::SoundComponent()
    {
        m_SoundNode = std::shared_ptr<SoundNode>(SoundNode::Create());
        m_Name = "Sound";
        m_BoundingShape = std::make_unique<Maths::BoundingSphere>(m_SoundNode->GetPosition(),m_SoundNode->GetRadius());
    }
    
	SoundComponent::SoundComponent(std::shared_ptr<SoundNode>& sound)
		: m_SoundNode(sound)
	{
		m_Name = "Sound";
		m_BoundingShape = std::make_unique<Maths::BoundingSphere>(sound->GetPosition(),sound->GetRadius());
	}

	void SoundComponent::OnUpdateComponent(float dt)
	{
	//	Physics3DComponent* physicsComponent = m_Entity->GetComponent<Physics3DComponent>();
	//	if (physicsComponent)
	//	{
	//		m_SoundNode->SetPosition(physicsComponent->GetPhysicsObject()->GetPosition()); //TODO : Get From Entity Transform
	//		////m_SoundNode->SetVelocity(physicsComponent->GetPhysicsObject()->GetLinearVelocity());
	//		m_BoundingShape->SetPosition(m_SoundNode->GetPosition());
	//	}
	}

	void SoundComponent::Init()
	{
		Application::Instance()->GetSystem<AudioManager>()->AddSoundNode(m_SoundNode.get());
	}

	void SoundComponent::OnIMGUI()
	{
		auto pos = m_SoundNode->GetPosition();
		auto radius = m_SoundNode->GetRadius();
		auto paused = m_SoundNode->GetPaused();
		auto pitch = m_SoundNode->GetPitch();
		auto referenceDistance = m_SoundNode->GetReferenceDistance();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2,2));
        ImGui::Columns(2);
        ImGui::Separator();
            
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Position");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::InputFloat3("##Position", &pos.x))
            m_SoundNode->SetPosition(pos);
            
        ImGui::PopItemWidth();
        ImGui::NextColumn();
            
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Radius");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::InputFloat("##Radius", &radius))
            m_SoundNode->SetRadius(radius);
            
        ImGui::PopItemWidth();
        ImGui::NextColumn();
            
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Pitch");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::InputFloat("##Pitch", &pitch))
            m_SoundNode->SetPitch(pitch);
            
        ImGui::PopItemWidth();
        ImGui::NextColumn();
            
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Reference Distance");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat("##Reference Distance", &referenceDistance))
            m_SoundNode->SetReferenceDistance(referenceDistance);
            
        ImGui::PopItemWidth();
        ImGui::NextColumn();
            
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Paused");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::Checkbox("##Paused", &paused))
            m_SoundNode->SetPaused(paused);
            
        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::Columns(1);
        ImGui::Separator();
        ImGui::PopStyleVar();
	}
}
