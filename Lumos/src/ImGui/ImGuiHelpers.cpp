#include "lmpch.h"
#include "ImGui/ImGuiHelpers.h"
#include "Graphics/API/Texture.h"
#include "Graphics/API/GraphicsContext.h"

#include <imgui/imgui.h>

namespace Lumos
{
	void ImGuiHelpers::Property(const String& name, bool& value)
	{
        ImGui::Text("%s", name.c_str());
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		String id = "##" + name;
		ImGui::Checkbox(id.c_str(), &value);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
	}

	void ImGuiHelpers::Property(const String& name, float& value, float min, float max, ImGuiHelpers::PropertyFlag flags)
	{
		ImGui::Text("%s", name.c_str());
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		String id = "##" + name;
		ImGui::SliderFloat(id.c_str(), &value, min, max);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
	}

	void ImGuiHelpers::Property(const String& name, Maths::Vector3& value, ImGuiHelpers::PropertyFlag flags)
	{
		ImGuiHelpers::Property(name, value, -1.0f, 1.0f, flags);
	}

	void ImGuiHelpers::Property(const String& name, Maths::Vector3& value, float min, float max, ImGuiHelpers::PropertyFlag flags)
	{
        ImGui::Text("%s", name.c_str());
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		String id = "##" + name;
		if ((int)flags & (int)PropertyFlag::ColorProperty)
			ImGui::ColorEdit3(id.c_str(), &value.x, ImGuiColorEditFlags_NoInputs);
		else
			ImGui::SliderFloat3(id.c_str(), &value.x, min, max);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
	}

	void ImGuiHelpers::Property(const String& name, Maths::Vector4& value, ImGuiHelpers::PropertyFlag flags)
	{
		Property(name, value, -1.0f, 1.0f, flags);
	}

	void ImGuiHelpers::Property(const String& name, Maths::Vector4& value, float min, float max, ImGuiHelpers::PropertyFlag flags)
	{
		ImGui::AlignTextToFramePadding();
        ImGui::Text("%s", name.c_str());
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		String id = "##" + name;
		if ((int)flags & (int)PropertyFlag::ColorProperty)
			ImGui::ColorEdit4(id.c_str(), &value.x, ImGuiColorEditFlags_NoInputs);
		else
			ImGui::SliderFloat4(id.c_str(), &value.x, min, max);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
	}

	void ImGuiHelpers::Tooltip(const String & text)
	{
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::TextUnformatted(text.c_str());
			ImGui::EndTooltip();
		}
	}

	void ImGuiHelpers::Tooltip(Graphics::Texture2D * texture, const Maths::Vector2 & size)
	{
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			bool flipImage = Graphics::GraphicsContext::GetContext()->FlipImGUITexture();
			ImGui::Image(texture ? texture->GetHandle() : nullptr, ImVec2(size.GetX(), size.GetY()), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
			ImGui::EndTooltip();
		}
	}

	void ImGuiHelpers::Tooltip(Graphics::Texture2D * texture, const Maths::Vector2 & size, const String & text)
	{
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			bool flipImage = Graphics::GraphicsContext::GetContext()->FlipImGUITexture();
			ImGui::Image(texture ? texture->GetHandle() : nullptr, ImVec2(size.GetX(), size.GetY()), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
			ImGui::TextUnformatted(text.c_str());
			ImGui::EndTooltip();
		}
	}

	void ImGuiHelpers::Image(Graphics::Texture2D * texture, const Maths::Vector2 & size)
	{
		bool flipImage = Graphics::GraphicsContext::GetContext()->FlipImGUITexture();
		ImGui::Image(texture ? texture->GetHandle() : nullptr, ImVec2(size.GetX(), size.GetY()), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
	}
}
