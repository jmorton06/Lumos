#include "LM.h"
#include "AIComponent.h"

#include <imgui/imgui.h>

namespace Lumos
{
	AIComponent::AIComponent(std::shared_ptr<AINode>& aiNode)
		: m_AINode(aiNode)
	{
		
	}

	void AIComponent::OnIMGUI()
	{
		if (ImGui::TreeNode("AI"))
		{
			ImGui::TreePop();
		}
	}

}
