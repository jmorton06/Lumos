#include "LM.h"
#include "TextureMatrixComponent.h"
#include <imgui/imgui.h>

namespace Lumos
{
	TextureMatrixComponent::TextureMatrixComponent(const maths::Matrix4& matrix)
		: m_TextureMatrix(matrix)
	{

	}

	void TextureMatrixComponent::OnIMGUI()
	{
		if (ImGui::TreeNode("TextureMatrix"))
		{
			ImGui::TreePop();
		}
	}

}
