#include "lmpch.h"
#include "Light.h"

#include "ImGui/ImGuiHelpers.h"
#include <imgui/imgui.h>

namespace Lumos
{
	namespace Graphics
	{
		Light::Light(const Maths::Vector3& direction, const Maths::Vector4& colour, float intensity, const LightType& type, const Maths::Vector3& position, float radius)
			: m_Direction(direction), m_Colour(colour), m_Position(position), m_Intensity(intensity), m_Radius(radius), m_Type(float(type))
		{
		}

		String LightTypeToString(Graphics::LightType type)
		{
			switch (type)
			{
			case Graphics::LightType::DirectionalLight: return "Directional Light";
			case Graphics::LightType::SpotLight: return "Spot Light";
			case Graphics::LightType::PointLight: return "Point Light";
			default: return "ERROR";
			}
		}

		void Light::OnImGui()
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
			ImGui::Columns(2);
			ImGui::Separator();

			ImGuiHelpers::Property("Position", m_Position);
			ImGuiHelpers::Property("Direction", m_Direction);
			ImGuiHelpers::Property("Radius", m_Radius, 0.0f, 100.0f);
			ImGuiHelpers::Property("Colour", m_Colour, ImGuiHelpers::PropertyFlag::ColorProperty);
			ImGuiHelpers::Property("Intensity", m_Intensity, 0.0f, 100.0f);

			ImGui::AlignTextToFramePadding();
			ImGui::Text("Light Type");
			ImGui::NextColumn();
			ImGui::PushItemWidth(-1);
			if (ImGui::BeginMenu(LightTypeToString(Graphics::LightType(int(m_Type))).c_str()))
			{
				if (ImGui::MenuItem("Directional Light", "", static_cast<int>(m_Type) == 0, true)) { m_Type = float(int(Graphics::LightType::DirectionalLight)); }
				if (ImGui::MenuItem("Spot Light", "", static_cast<int>(m_Type) == 1, true)) { m_Type = float(int(Graphics::LightType::SpotLight)); }
				if (ImGui::MenuItem("Point Light", "", static_cast<int>(m_Type) == 2, true)) { m_Type = float(int(Graphics::LightType::PointLight)); }
				ImGui::EndMenu();
			}

			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::Columns(1);
			ImGui::Separator();
			ImGui::PopStyleVar();
		}

		nlohmann::json Light::Serialise()
		{
			nlohmann::json output;
			output["typeID"] = LUMOS_TYPENAME(Light);
			output["position"] = m_Position.Serialise();
			output["direction"] = m_Direction.Serialise();
			output["colour"] = m_Colour.Serialise();
			output["intensity"] = m_Intensity;
			output["radius"] = m_Radius;
			output["type"] = m_Type;

			return output;
		}

		void Light::Deserialise(nlohmann::json & data)
		{
			m_Position.Deserialise(data["position"]);
			m_Direction.Deserialise(data["direction"]);
			m_Colour.Deserialise(data["colour"]);
			m_Intensity = data["intensity"];
			m_Radius = data["radius"];
			m_Type = data["type"];
		}
	}
}

