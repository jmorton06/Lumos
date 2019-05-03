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
        m_Transform.SetWorldMatrix(m_Transform.GetLocalMatrix() * matrix);
    }

    void TransformComponent::OnIMGUI()
    {
        if (ImGui::TreeNode("Transform"))
        {
            auto localMatrix = m_Transform.GetLocalMatrix();
			auto pos = localMatrix.GetPositionVector();
			auto scale = localMatrix.GetScaling();
            auto rotation = maths::Matrix4::GetEulerAngles(localMatrix);

			ImGui::InputFloat3("Position", &pos.x);
			ImGui::InputFloat3("Scale", &scale.x);
            ImGui::InputFloat3("Rotation", &rotation.x);
            
			m_Transform.SetLocalPosition(pos);
			m_Transform.SetLocalScale(scale);
            
            auto quat = maths::Quaternion(rotation,1.0f);
            quat.GenerateW();
            
            m_Transform.SetLocalOrientation(quat);

            ImGui::TreePop();
        }
    }

}
