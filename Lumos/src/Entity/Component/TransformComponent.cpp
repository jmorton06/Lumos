#include "LM.h"
#include "TransformComponent.h"

#include <imgui/imgui.h>

namespace Lumos
{
	TransformComponent::TransformComponent(const maths::Matrix4& matrix)
		: m_WorldSpaceTransform(matrix) , m_LocalTransform(matrix)
	{

	}
    
    void TransformComponent::OnIMGUI()
    {
        if (ImGui::TreeNode("Transform"))
        {
			auto pos = m_WorldSpaceTransform.GetPositionVector();
			auto scale = m_WorldSpaceTransform.GetScaling();

			ImGui::DragFloat3("Position", &pos.x);
			ImGui::DragFloat3("Scale", &scale.x);
            ImGui::TreePop();
        }
    }

}
