#include "LM.h"
#include "TransformComponent.h"

#include <imgui/imgui.h>

namespace Lumos
{
	TransformComponent::TransformComponent(const maths::Matrix4& matrix)
		: m_Transform(matrix)
	{

	}
    
    void TransformComponent::OnIMGUI()
    {
        if (ImGui::TreeNode("Transform"))
        {
			auto pos = m_Transform.GetWorldMatrix().GetPositionVector();
			auto scale = m_Transform.GetWorldMatrix().GetScaling();

			ImGui::DragFloat3("Position", &pos.x);
			ImGui::DragFloat3("Scale", &scale.x);
            ImGui::TreePop();
        }
    }

}
