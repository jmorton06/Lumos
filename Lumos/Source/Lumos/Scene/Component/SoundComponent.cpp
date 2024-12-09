#include "Precompiled.h"
#include "SoundComponent.h"
#include "Scene/Scene.h"
#include "Core/Application.h"
#include "Audio/AudioManager.h"
#include "Maths/Vector3.h"
#include "Maths/MathsUtilities.h"
#include <imgui/imgui.h>

namespace Lumos
{
    SoundComponent::SoundComponent()
    {
        m_SoundNode = SharedPtr<SoundNode>(SoundNode::Create());
    }

    SoundComponent::SoundComponent(SharedPtr<SoundNode>& sound)
        : m_SoundNode(sound)
    {
    }

    void SoundComponent::Init()
    {
    }

    void SoundComponent::OnImGui()
    {
        auto pos               = m_SoundNode->GetPosition();
        auto radius            = m_SoundNode->GetRadius();
        auto paused            = m_SoundNode->GetPaused();
        auto pitch             = m_SoundNode->GetPitch();
        auto referenceDistance = m_SoundNode->GetReferenceDistance();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::Columns(2);
        ImGui::Separator();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Position");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::InputFloat3("##Position", Maths::ValuePtr(pos)))
            m_SoundNode->SetPosition(pos);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Radius");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::InputFloat("##Radius", &radius))
            m_SoundNode->SetRadius(radius);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Pitch");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::InputFloat("##Pitch", &pitch))
            m_SoundNode->SetPitch(pitch);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Reference Distance");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat("##Reference Distance", &referenceDistance))
            m_SoundNode->SetReferenceDistance(referenceDistance);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Paused");
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
