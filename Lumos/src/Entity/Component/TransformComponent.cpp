#include "LM.h"
#include "TransformComponent.h"
#include "Entity/Entity.h"

#include <imgui/imgui.h>
#include <imgui/plugins/ImGuizmo.h>

namespace Lumos
{
	TransformComponent::TransformComponent(const Maths::Matrix4& matrix)
		: m_Transform(matrix)
	{
		m_Name = "Transform";
		m_CanDisable = false;
	}

	TransformComponent::TransformComponent()
		: m_Transform()
	{
		m_Name = "Transform";
		m_CanDisable = false;
	}

	void TransformComponent::OnUpdateComponent(float dt)
	{
	}
    
    void TransformComponent::SetWorldMatrix(const Maths::Matrix4 &matrix)
    {
        m_Transform.SetHasUpdated(true);
        m_Transform.SetWorldMatrix(matrix);
    }

    void TransformComponent::OnIMGUI()
    {
		auto pos = m_Transform.GetLocalPosition();
		auto scale = m_Transform.GetLocalScale();
        auto rotation = m_Transform.GetLocalOrientation().ToEuler();
            
        bool update = false;

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2,2));
        ImGui::Columns(2);
        ImGui::Separator();
            
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Position");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::InputFloat3("##Position", &pos.x))
        {
            m_Transform.SetLocalPosition(pos);
            update = true;
        }
            
        ImGui::PopItemWidth();
        ImGui::NextColumn();
            
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Rotation");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::InputFloat3("##Rotation", &rotation.x))
        {
            m_Transform.SetLocalOrientation(Maths::Quaternion::EulerAnglesToQuaternion(rotation.GetX(), rotation.GetY(), rotation.GetZ()));
            update = true;
        }
            
        ImGui::PopItemWidth();
        ImGui::NextColumn();
            
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Scale");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::InputFloat3("##Scale", &scale.x))
        {
            m_Transform.SetLocalScale(scale);
            update = true;
        }
            
        ImGui::PopItemWidth();
        ImGui::NextColumn();
            
        if(update)
            m_Transform.UpdateMatrices();

        ImGui::Columns(1);
        ImGui::Separator();
        ImGui::PopStyleVar();
    }
}
