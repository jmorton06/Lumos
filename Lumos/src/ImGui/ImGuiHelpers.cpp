#include "lmpch.h"
#include "ImGui/ImGuiHelpers.h"

namespace Lumos
{
	void ImGuiHelpers::Property(const String& name, bool& value)
	{
		ImGui::Text(name.c_str());
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		String id = "##" + name;
		ImGui::Checkbox(id.c_str(), &value);

		ImGui::PopItemWidth();
		ImGui::NextColumn();
	}

	void ImGuiHelpers::Property(const String& name, float& value, float min, float max, ImGuiHelpers::PropertyFlag flags)
	{
		ImGui::Text(name.c_str());
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
		ImGui::Text(name.c_str());
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
		ImGui::Text(name.c_str());
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
}