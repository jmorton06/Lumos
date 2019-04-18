#include "LM.h"
#include "Light.h"
#include <imgui/imgui.h>

String Lumos::Light::GetName()
{
	switch (m_Type)
	{
	case LightType::DirectionalLight: return "Directional";
	case LightType::PointLight: return "Point";
	case LightType::SpotLight: return "Spot";
	default: return "Light";
	}
}

void Lumos::Light::OnImGUI()
{
	if (ImGui::TreeNode(GetName().c_str()))
	{
		ImGui::DragFloat3("Position", &m_Position.x);
		ImGui::DragFloat3("Direction", &m_Direction.x);
		ImGui::DragFloat3("Colour", &m_Colour.x);
		ImGui::Checkbox("IsOn", &m_IsOn);
		ImGui::DragFloat("Brightness", &m_Brightness);
		ImGui::DragFloat("Radius", &m_Radius);

		ImGui::TreePop();
	}
}
