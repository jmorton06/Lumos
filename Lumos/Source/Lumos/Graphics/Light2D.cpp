#include "Precompiled.h"
#include "Light2D.h"

#include "ImGui/ImGuiUtilities.h"
#include <imgui/imgui.h>

namespace Lumos
{
    namespace Graphics
    {
        Light2D::Light2D(const Vec4& colour, float intensity, float radius, const Light2DType& type)
            : Colour(colour)
            , Position(Vec4(0.0f))
            , Direction(Vec4(0.0f, -1.0f, 0.0f, 0.0f))
            , Intensity(intensity)
            , Radius(radius)
            , Type(float(type))
            , Height(1.0f)
            , InnerAngle(0.9f)
            , OuterAngle(0.8f)
            , Falloff(1.0f)
            , _pad(0.0f)
        {
        }

        std::string Light2D::LightTypeToString(Graphics::Light2DType type)
        {
            switch(type)
            {
            case Graphics::Light2DType::Point:
                return "Point Light";
            case Graphics::Light2DType::Spot:
                return "Spot Light";
            case Graphics::Light2DType::Global:
                return "Global Light";
            default:
                return "ERROR";
            }
        }

        void Light2D::OnImGui()
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            ImGui::Columns(2);
            ImGui::Separator();

            ImGuiUtilities::Property("Colour", Colour, true, ImGuiUtilities::PropertyFlag::ColourProperty);
            ImGuiUtilities::Property("Intensity", Intensity, 0.0f, 50.0f);

            if(Type != float(Light2DType::Global))
                ImGuiUtilities::Property("Radius", Radius, 0.0f, 100.0f);

            ImGuiUtilities::Property("Height", Height, 0.0f, 20.0f);
            ImGuiUtilities::Property("Falloff", Falloff, 0.1f, 8.0f);

            if(Type == float(Light2DType::Spot))
            {
                ImGuiUtilities::Property("Inner Angle", InnerAngle, -1.0f, 1.0f);
                ImGuiUtilities::Property("Outer Angle", OuterAngle, -1.0f, 1.0f);
            }

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Light Type");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if(ImGui::BeginMenu(LightTypeToString(Graphics::Light2DType(int(Type))).c_str()))
            {
                if(ImGui::MenuItem("Point Light", "", static_cast<int>(Type) == 0, true))
                    Type = float(Graphics::Light2DType::Point);
                if(ImGui::MenuItem("Spot Light", "", static_cast<int>(Type) == 1, true))
                    Type = float(Graphics::Light2DType::Spot);
                if(ImGui::MenuItem("Global Light", "", static_cast<int>(Type) == 2, true))
                    Type = float(Graphics::Light2DType::Global);
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
