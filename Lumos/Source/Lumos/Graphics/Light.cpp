#include "Precompiled.h"
#include "Light.h"

#include "ImGui/ImGuiHelpers.h"
#include <imgui/imgui.h>

namespace Lumos
{
    namespace Graphics
    {
        Light::Light(const Maths::Vector3& direction, const Maths::Vector4& colour, float intensity, const LightType& type, const Maths::Vector3& position, float radius, float angle)
            : Direction(direction)
            , Colour(colour)
            , Position(position)
            , Intensity(intensity)
            , Radius(radius)
            , Type(float(type))
            , Angle(angle)
        {
        }

        std::string Light::LightTypeToString(Graphics::LightType type)
        {
            switch(type)
            {
            case Graphics::LightType::DirectionalLight:
                return "Directional Light";
            case Graphics::LightType::SpotLight:
                return "Spot Light";
            case Graphics::LightType::PointLight:
                return "Point Light";
            default:
                return "ERROR";
            }
        }

        float Light::StringToLightType(const std::string& type)
        {
            if(type == "Directional")
                return float(Graphics::LightType::DirectionalLight);

            if(type == "Point")
                return float(Graphics::LightType::PointLight);

            if(type == "Spot")
                return float(Graphics::LightType::SpotLight);

            LUMOS_LOG_ERROR("Unknown Light Type");
            return 0.0f;
        }

        void Light::OnImGui()
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            ImGui::Columns(2);
            ImGui::Separator();

            if(Type != 0)
                ImGuiHelpers::Property("Position", Position);

            if(Type != 2)
                ImGuiHelpers::Property("Direction", Direction);

            if(Type != 0)
                ImGuiHelpers::Property("Radius", Radius, 0.0f, 100.0f);
            ImGuiHelpers::Property("Colour", Colour, true, ImGuiHelpers::PropertyFlag::ColourProperty);
            ImGuiHelpers::Property("Intensity", Intensity, 0.0f, 100.0f);

            if(Type == 1)
                ImGuiHelpers::Property("Angle", Angle, -1.0f, 1.0f);

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Light Type");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if(ImGui::BeginMenu(LightTypeToString(Graphics::LightType(int(Type))).c_str()))
            {
                if(ImGui::MenuItem("Directional Light", "", static_cast<int>(Type) == 0, true))
                {
                    Type = float(int(Graphics::LightType::DirectionalLight));
                }
                if(ImGui::MenuItem("Spot Light", "", static_cast<int>(Type) == 1, true))
                {
                    Type = float(int(Graphics::LightType::SpotLight));
                }
                if(ImGui::MenuItem("Point Light", "", static_cast<int>(Type) == 2, true))
                {
                    Type = float(int(Graphics::LightType::PointLight));
                }
                ImGui::EndMenu();
            }

            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::Columns(1);
            ImGui::Separator();
            ImGui::PopStyleVar();
        }
    }
}
