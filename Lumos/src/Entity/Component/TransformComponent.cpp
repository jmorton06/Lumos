#include "LM.h"
#include "TransformComponent.h"
#include "Entity/Entity.h"

#include <imgui/imgui.h>

namespace Lumos
{
	TransformComponent::TransformComponent(const maths::Matrix4& matrix)
		: m_Transform(matrix)
	{

	}

	void TransformComponent::OnUpdateComponent(float dt)
	{
	}
    
    void TransformComponent::SetWorldMatrix(const maths::Matrix4 &matrix)
    {
        m_Transform.SetHasUpdated(true);
        m_Transform.SetWorldMatrix(matrix);
    }

    void TransformComponent::OnIMGUI()
    {
        if (ImGui::TreeNode("Transform"))
        {
            auto localMatrix = m_Transform.GetLocalMatrix();
			auto pos = localMatrix.GetPositionVector();
			auto scale = localMatrix.GetScaling();
            auto rotation = maths::Matrix4::GetEulerAngles(localMatrix);
            
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
                auto quat = maths::Quaternion(rotation,1.0f);
                quat.GenerateW();
                
                m_Transform.SetLocalOrientation(quat);
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
            
            ImGui::TreePop();
        }
    }
}
