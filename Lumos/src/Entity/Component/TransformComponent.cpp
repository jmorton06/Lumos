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
            ImGui::TreePop();
        }
    }

}
