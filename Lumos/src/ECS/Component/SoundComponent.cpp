#include "LM.h"
#include "SoundComponent.h"
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
        m_SoundNode = Ref<SoundNode>(SoundNode::Create());
        m_Name = "Sound";
        m_BoundingShape = CreateScope<Maths::BoundingSphere>(m_SoundNode->GetPosition(),m_SoundNode->GetRadius());
    }
    
	SoundComponent::SoundComponent(Ref<SoundNode>& sound)
		: m_SoundNode(sound)
	{
		m_Name = "Sound";
		m_BoundingShape = CreateScope<Maths::BoundingSphere>(sound->GetPosition(),sound->GetRadius());
	}

	void SoundComponent::Init()
	{
		Application::Instance()->GetSystem<AudioManager>()->AddSoundNode(m_SoundNode.get());
	}

	void SoundComponent::OnImGui()
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
