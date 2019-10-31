#include "lmpch.h"
#include "ImGui/ImGuiHelpers.h"
#include "Graphics/API/Texture.h"
#include "Graphics/API/GraphicsContext.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

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
			ImGui::ColorEdit3(id.c_str(), Maths::ValuePointer(value), ImGuiColorEditFlags_NoInputs);
		else
			ImGui::SliderFloat3(id.c_str(), Maths::ValuePointer(value), min, max);

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
			ImGui::ColorEdit4(id.c_str(), Maths::ValuePointer(value), ImGuiColorEditFlags_NoInputs);
		else
			ImGui::SliderFloat4(id.c_str(), Maths::ValuePointer(value), min, max);

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

    bool ImGuiHelpers::BufferingBar(const char* label, float value,  const Maths::Vector2& size_arg, const u32& bg_col, const u32& fg_col)
    {
        auto g = ImGui::GetCurrentContext();
        auto drawList = ImGui::GetWindowDrawList();
        const ImGuiStyle& style = ImGui::GetStyle();
        const ImGuiID id = ImGui::GetID(label);

        ImVec2 pos = ImGui::GetCursorPos();
        ImVec2 size = { size_arg.x , size_arg.y };
        size.x -= style.FramePadding.x * 2;
        
        const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
        ImGui::ItemSize(bb, style.FramePadding.y);
        if (!ImGui::ItemAdd(bb, id))
            return false;
        
        // Render
        const float circleStart = size.x * 0.7f;
        const float circleEnd = size.x;
        const float circleWidth = circleEnd - circleStart;
        
        drawList->AddRectFilled(bb.Min, ImVec2(pos.x + circleStart, bb.Max.y), bg_col);
        drawList->AddRectFilled(bb.Min, ImVec2(pos.x + circleStart*value, bb.Max.y), fg_col);
        
        const float t = float(g->Time);
        const float r = size.y / 2.0f;
        const float speed = 1.5f;
        
        const float a = speed*0.f;
        const float b = speed*0.333f;
        const float c = speed*0.666f;
        
        const float o1 = (circleWidth+r) * (t+a - speed * (int)((t+a) / speed)) / speed;
        const float o2 = (circleWidth+r) * (t+b - speed * (int)((t+b) / speed)) / speed;
        const float o3 = (circleWidth+r) * (t+c - speed * (int)((t+c) / speed)) / speed;
        
        drawList->AddCircleFilled(ImVec2(pos.x + circleEnd - o1, bb.Min.y + r), r, bg_col);
        drawList->AddCircleFilled(ImVec2(pos.x + circleEnd - o2, bb.Min.y + r), r, bg_col);
        drawList->AddCircleFilled(ImVec2(pos.x + circleEnd - o3, bb.Min.y + r), r, bg_col);
        
        return true;
    }

    bool ImGuiHelpers::Spinner(const char* label, float radius, int thickness, const u32& color)
    {
        auto g = ImGui::GetCurrentContext();
        const ImGuiStyle& style = g->Style;
        const ImGuiID id = ImGui::GetID(label);
        auto drawList = ImGui::GetWindowDrawList();

        ImVec2 pos = ImGui::GetCursorPos();
        ImVec2 size((radius )*2, (radius + style.FramePadding.y)*2);
        
        const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
        ImGui::ItemSize(bb, style.FramePadding.y);
        if (!ImGui::ItemAdd(bb, id))
            return false;
        
        // Render
        drawList->PathClear();
        
        int num_segments = 30;
        float start = abs(ImSin(float(g->Time)*1.8f)*(num_segments-5));
        
        const float a_min = IM_PI*2.0f * (start / float(num_segments));
        const float a_max = IM_PI*2.0f * (float(num_segments)-3.0f) / (float)num_segments;

        const ImVec2 centre = ImVec2(pos.x+radius, pos.y+radius+style.FramePadding.y);
        
        for (int i = 0; i < num_segments; i++) {
            const float a = a_min + (float(i) / float(num_segments)) * (a_max - a_min);
            drawList->PathLineTo(ImVec2(centre.x + ImCos(a+float(g->Time)*8) * radius,
                                                centre.y + ImSin(a+ float(g->Time) *8) * radius));
        }

        drawList->PathStroke(color, false, float(thickness));
        
        return true;
    }


	void ImGuiHelpers::SetTheme(Theme theme)
	{
        ImVec4 color_for_text = ImVec4(236.f / 255.f, 240.f / 255.f, 241.f / 255.f,1.0f);
        ImVec4 color_for_head = ImVec4(41.f / 255.f, 128.f / 255.f, 185.f / 255.f,1.0f);
        ImVec4 color_for_area = ImVec4(57.f / 255.f, 79.f / 255.f, 105.f / 255.f,1.0f);
        ImVec4 color_for_body = ImVec4(44.f / 255.f, 62.f / 255.f, 80.f / 255.f,1.0f);
        ImVec4 color_for_pops = ImVec4(33.f / 255.f, 46.f / 255.f, 60.f / 255.f,1.0f);

		ImVec4 black(0, 0, 0, 0);
		ImVec4 white(1, 1, 1, 1);
		ImVec4 color1(0.86f, 0.93f, 0.89f, 1); // text ->
		ImVec4 color2(0.20f, 0.22f, 0.27f, 1); // blur
		ImVec4 color3(0.92f, 0.18f, 0.29f, 1); // active
		ImVec4 color4(0.47f, 0.77f, 0.83f, 1); // slider ->
		ImVec4 color5(0.13f, 0.14f, 0.17f, 1); // windowbg
		ImVec4 color6(0.31f, 0.31f, 1.00f, 1); // border, unused -> black
		ImVec4 color7(0.09f, 0.15f, 0.16f, 1); // ScrollbarGrab
		ImVec4 color8(0.71f, 0.22f, 0.27f, 1); // CheckMark
		ImVec4 color9(0.14f, 0.16f, 0.19f, 1); // Column

		auto mul = [](ImVec4 c, float t) -> ImVec4 { return ImVec4(c.x * t, c.y * t, c.z * t, c.w); };
		auto lum = [](ImVec4 c, float x) -> ImVec4 { return ImVec4(c.x, c.y, c.z, c.w * x); };

		ImVec4* colours = ImGui::GetStyle().Colors;
		switch (theme)
		{
		case Lumos::ImGuiHelpers::Black:
			ImGui::StyleColorsDark();
			colours[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
			colours[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
			colours[ImGuiCol_WindowBg] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
			colours[ImGuiCol_ChildBg] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
			colours[ImGuiCol_PopupBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
			colours[ImGuiCol_Border] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
			colours[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
			colours[ImGuiCol_FrameBg] = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
			colours[ImGuiCol_FrameBgHovered] = ImVec4(0.32f, 0.32f, 0.32f, 1.00f);
			colours[ImGuiCol_FrameBgActive] = ImVec4(0.37f, 0.37f, 0.37f, 1.00f);
			colours[ImGuiCol_TitleBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
			colours[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
			colours[ImGuiCol_TitleBgCollapsed] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
			colours[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
			colours[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.00f);
			colours[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
			colours[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
			colours[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
			colours[ImGuiCol_CheckMark] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
			colours[ImGuiCol_SliderGrab] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
			colours[ImGuiCol_SliderGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 1.00f);
			colours[ImGuiCol_Button] = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
			colours[ImGuiCol_ButtonHovered] = ImVec4(0.34f, 0.34f, 0.34f, 1.00f);
			colours[ImGuiCol_ButtonActive] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
			colours[ImGuiCol_Header] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
			colours[ImGuiCol_HeaderHovered] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
			colours[ImGuiCol_HeaderActive] = ImVec4(0.44f, 0.44f, 0.44f, 1.00f);
			colours[ImGuiCol_Separator] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
			colours[ImGuiCol_SeparatorHovered] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
			colours[ImGuiCol_SeparatorActive] = ImVec4(0.34f, 0.34f, 0.34f, 1.00f);
			colours[ImGuiCol_ResizeGrip] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
			colours[ImGuiCol_ResizeGripHovered] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
			colours[ImGuiCol_ResizeGripActive] = ImVec4(0.37f, 0.37f, 0.37f, 1.00f);
			colours[ImGuiCol_Tab] = ImVec4(0.26f, 0.26f, 0.26f, 0.40f);
			colours[ImGuiCol_TabHovered] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
			colours[ImGuiCol_TabActive] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
			colours[ImGuiCol_TabUnfocused] = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);
			colours[ImGuiCol_TabUnfocusedActive] = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
			colours[ImGuiCol_DockingPreview] = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);
			colours[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
			colours[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
			colours[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
			colours[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
			colours[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
			colours[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
			colours[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
			colours[ImGuiCol_NavHighlight] = ImVec4(0.78f, 0.88f, 1.00f, 1.00f);
			colours[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
			colours[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
			colours[ImGuiCol_ModalWindowDimBg] = ImVec4(0.44f, 0.44f, 0.44f, 0.35f);
			break;
		case Lumos::ImGuiHelpers::Dark:
			ImGui::StyleColorsDark();
			colours[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
			colours[ImGuiCol_TextDisabled] = ImVec4(0.36f, 0.42f, 0.47f, 1.00f);
			colours[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
			colours[ImGuiCol_ChildBg] = ImVec4(27.0f/255.0f,32.0f/255.0f,46.0f/255.0f, 1.00f);
			colours[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
			colours[ImGuiCol_Border] = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
			colours[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
			colours[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
			colours[ImGuiCol_FrameBgHovered] = ImVec4(0.12f, 0.20f, 0.28f, 1.00f);
			colours[ImGuiCol_FrameBgActive] = ImVec4(0.09f, 0.12f, 0.14f, 1.00f);
			colours[ImGuiCol_TitleBg] = colours[ImGuiCol_WindowBg];// ImVec4(0.09f, 0.12f, 0.14f, 0.65f);
			colours[ImGuiCol_TitleBgActive] = colours[ImGuiCol_WindowBg];// ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
			colours[ImGuiCol_TitleBgCollapsed] = colours[ImGuiCol_WindowBg];// ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
			colours[ImGuiCol_MenuBarBg] = colours[ImGuiCol_WindowBg];
			colours[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.39f);
			colours[ImGuiCol_ScrollbarGrab] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
			colours[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.18f, 0.22f, 0.25f, 1.00f);
			colours[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.09f, 0.21f, 0.31f, 1.00f);
			colours[ImGuiCol_CheckMark] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
			colours[ImGuiCol_SliderGrab] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
			colours[ImGuiCol_SliderGrabActive] = ImVec4(0.37f, 0.61f, 1.00f, 1.00f);
			colours[ImGuiCol_Button] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
			colours[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
			colours[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
			colours[ImGuiCol_Header] = ImVec4(0.20f, 0.25f, 0.29f, 0.55f);
			colours[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
			colours[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
			colours[ImGuiCol_Separator] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
			colours[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
			colours[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
			colours[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
			colours[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
			colours[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
			colours[ImGuiCol_Tab] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
			colours[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
			colours[ImGuiCol_TabActive] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
			colours[ImGuiCol_TabUnfocused] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
			colours[ImGuiCol_TabUnfocusedActive] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
			colours[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
			colours[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
			colours[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
			colours[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
			colours[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
			colours[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
			colours[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
			colours[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
			colours[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
			colours[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

#ifdef IMGUI_HAS_DOCK
            colours[ImGuiCol_TabActive] = colours[ImGuiCol_ChildBg];  

			//colours[ImGuiCol_DockingEmptyBg] = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
			//colours[ImGuiCol_Tab] = ImVec4(0.09f, 0.21f, 0.31f, 1.00f);
			//colours[ImGuiCol_TabHovered] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
			//colours[ImGuiCol_TabActive] = ImVec4(0.18f, 0.22f, 0.25f, 1.00f);
			//colours[ImGuiCol_TabUnfocused] = ImVec4(0.09f, 0.21f, 0.31f, 1.00f);
			//colours[ImGuiCol_TabUnfocusedActive] = ImVec4(0.18f, 0.22f, 0.25f, 1.00f);
			//colours[ImGuiCol_DockingPreview] = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);

#endif
			break;
		case Lumos::ImGuiHelpers::Grey:
			ImGui::StyleColorsDark();
			colours[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
			colours[ImGuiCol_TextDisabled] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
			colours[ImGuiCol_ChildBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
			colours[ImGuiCol_WindowBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
			colours[ImGuiCol_PopupBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
			colours[ImGuiCol_Border] = ImVec4(0.12f, 0.12f, 0.12f, 0.71f);
			colours[ImGuiCol_BorderShadow] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
			colours[ImGuiCol_FrameBg] = ImVec4(0.42f, 0.42f, 0.42f, 0.54f);
			colours[ImGuiCol_FrameBgHovered] = ImVec4(0.42f, 0.42f, 0.42f, 0.40f);
			colours[ImGuiCol_FrameBgActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.67f);
			colours[ImGuiCol_TitleBg] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
			colours[ImGuiCol_TitleBgActive] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
			colours[ImGuiCol_TitleBgCollapsed] = ImVec4(0.17f, 0.17f, 0.17f, 0.90f);
			colours[ImGuiCol_MenuBarBg] = ImVec4(0.335f, 0.335f, 0.335f, 1.000f);
			colours[ImGuiCol_ScrollbarBg] = ImVec4(0.24f, 0.24f, 0.24f, 0.53f);
			colours[ImGuiCol_ScrollbarGrab] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
			colours[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
			colours[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
			colours[ImGuiCol_CheckMark] = ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
			colours[ImGuiCol_SliderGrab] = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
			colours[ImGuiCol_SliderGrabActive] = ImVec4(0.64f, 0.64f, 0.64f, 1.00f);
			colours[ImGuiCol_Button] = ImVec4(0.54f, 0.54f, 0.54f, 0.35f);
			colours[ImGuiCol_ButtonHovered] = ImVec4(0.52f, 0.52f, 0.52f, 0.59f);
			colours[ImGuiCol_ButtonActive] = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
			colours[ImGuiCol_Header] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
			colours[ImGuiCol_HeaderHovered] = ImVec4(0.47f, 0.47f, 0.47f, 1.00f);
			colours[ImGuiCol_HeaderActive] = ImVec4(0.76f, 0.76f, 0.76f, 0.77f);
			colours[ImGuiCol_Separator] = ImVec4(0.000f, 0.000f, 0.000f, 0.137f);
			colours[ImGuiCol_SeparatorHovered] = ImVec4(0.700f, 0.671f, 0.600f, 0.290f);
			colours[ImGuiCol_SeparatorActive] = ImVec4(0.702f, 0.671f, 0.600f, 0.674f);
			colours[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
			colours[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
			colours[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
			colours[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
			colours[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
			colours[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
			colours[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
			colours[ImGuiCol_TextSelectedBg] = ImVec4(0.73f, 0.73f, 0.73f, 0.35f);
			colours[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
			colours[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
			colours[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
			colours[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
			colours[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);

#ifdef IMGUI_HAS_DOCK 
			colours[ImGuiCol_DockingEmptyBg] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
			colours[ImGuiCol_Tab] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
			colours[ImGuiCol_TabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
			colours[ImGuiCol_TabActive] = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
			colours[ImGuiCol_TabUnfocused] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
			colours[ImGuiCol_TabUnfocusedActive] = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
			colours[ImGuiCol_DockingPreview] = ImVec4(0.85f, 0.85f, 0.85f, 0.28f);
#endif
			break;
		case Lumos::ImGuiHelpers::Light:
			ImGui::StyleColorsLight();
			colours[ImGuiCol_Text] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
			colours[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
			colours[ImGuiCol_WindowBg] = ImVec4(0.94f, 0.94f, 0.94f, 0.94f);
			colours[ImGuiCol_PopupBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
			colours[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.39f);
			colours[ImGuiCol_BorderShadow] = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
			colours[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
			colours[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
			colours[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
			colours[ImGuiCol_TitleBg] = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
			colours[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
			colours[ImGuiCol_TitleBgActive] = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
			colours[ImGuiCol_MenuBarBg] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
			colours[ImGuiCol_ScrollbarBg] = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
			colours[ImGuiCol_ScrollbarGrab] = ImVec4(0.69f, 0.69f, 0.69f, 1.00f);
			colours[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.59f, 0.59f, 0.59f, 1.00f);
			colours[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
			colours[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
			colours[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
			colours[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
			colours[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
			colours[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
			colours[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
			colours[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
			colours[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
			colours[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
			colours[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.50f);
			colours[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
			colours[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
			colours[ImGuiCol_PlotLines] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
			colours[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
			colours[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
			colours[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
			colours[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
			break;
		case Lumos::ImGuiHelpers::Cherry:
			ImGui::StyleColorsDark();
#define HI(v)   ImVec4(0.502f, 0.075f, 0.256f, v)
#define MED(v)  ImVec4(0.455f, 0.198f, 0.301f, v)
#define LOW(v)  ImVec4(0.232f, 0.201f, 0.271f, v)
#define BG(v)   ImVec4(0.200f, 0.220f, 0.270f, v)
#define TEXTCol(v) ImVec4(0.860f, 0.930f, 0.890f, v)

			colours[ImGuiCol_Text] = TEXTCol(0.78f);
			colours[ImGuiCol_TextDisabled] = TEXTCol(0.28f);
			colours[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.14f, 0.17f, 1.00f);
			colours[ImGuiCol_PopupBg] = BG(0.9f);
			colours[ImGuiCol_Border] = ImVec4(0.31f, 0.31f, 1.00f, 0.00f);
			colours[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
			colours[ImGuiCol_FrameBg] = BG(1.00f);
			colours[ImGuiCol_FrameBgHovered] = MED(0.78f);
			colours[ImGuiCol_FrameBgActive] = MED(1.00f);
			colours[ImGuiCol_TitleBg] = LOW(1.00f);
			colours[ImGuiCol_TitleBgActive] = HI(1.00f);
			colours[ImGuiCol_TitleBgCollapsed] = BG(0.75f);
			colours[ImGuiCol_MenuBarBg] = BG(0.47f);
			colours[ImGuiCol_ScrollbarBg] = BG(1.00f);
			colours[ImGuiCol_ScrollbarGrab] = ImVec4(0.09f, 0.15f, 0.16f, 1.00f);
			colours[ImGuiCol_ScrollbarGrabHovered] = MED(0.78f);
			colours[ImGuiCol_ScrollbarGrabActive] = MED(1.00f);
			colours[ImGuiCol_CheckMark] = ImVec4(0.71f, 0.22f, 0.27f, 1.00f);
			colours[ImGuiCol_SliderGrab] = ImVec4(0.47f, 0.77f, 0.83f, 0.14f);
			colours[ImGuiCol_SliderGrabActive] = ImVec4(0.71f, 0.22f, 0.27f, 1.00f);
			colours[ImGuiCol_Button] = ImVec4(0.47f, 0.77f, 0.83f, 0.14f);
			colours[ImGuiCol_ButtonHovered] = MED(0.86f);
			colours[ImGuiCol_ButtonActive] = MED(1.00f);
			colours[ImGuiCol_Header] = MED(0.76f);
			colours[ImGuiCol_HeaderHovered] = MED(0.86f);
			colours[ImGuiCol_HeaderActive] = HI(1.00f);
			colours[ImGuiCol_ResizeGrip] = ImVec4(0.47f, 0.77f, 0.83f, 0.04f);
			colours[ImGuiCol_ResizeGripHovered] = MED(0.78f);
			colours[ImGuiCol_ResizeGripActive] = MED(1.00f);
			colours[ImGuiCol_PlotLines] = TEXTCol(0.63f);
			colours[ImGuiCol_PlotLinesHovered] = MED(1.00f);
			colours[ImGuiCol_PlotHistogram] = TEXTCol(0.63f);
			colours[ImGuiCol_PlotHistogramHovered] = MED(1.00f);
			colours[ImGuiCol_TextSelectedBg] = MED(0.43f);
			colours[ImGuiCol_Border] = ImVec4(0.539f, 0.479f, 0.255f, 0.162f);
			break;
        case Blue:
            colours[ImGuiCol_Text] = ImVec4(color_for_text.x, color_for_text.y, color_for_text.z, 1.00f);
            colours[ImGuiCol_TextDisabled] = ImVec4(color_for_text.x, color_for_text.y, color_for_text.z, 0.58f);
            colours[ImGuiCol_WindowBg] = ImVec4(color_for_body.x, color_for_body.y, color_for_body.z, 0.95f);
            colours[ImGuiCol_ChildWindowBg] = ImVec4(color_for_area.x, color_for_area.y, color_for_area.z, 0.58f);
            colours[ImGuiCol_Border] = ImVec4(color_for_body.x, color_for_body.y, color_for_body.z, 0.00f);
            colours[ImGuiCol_BorderShadow] = ImVec4(color_for_body.x, color_for_body.y, color_for_body.z, 0.00f);
            colours[ImGuiCol_FrameBg] = ImVec4(color_for_area.x, color_for_area.y, color_for_area.z, 1.00f);
            colours[ImGuiCol_FrameBgHovered] = ImVec4(color_for_head.x, color_for_head.y, color_for_head.z, 0.78f);
            colours[ImGuiCol_FrameBgActive] = ImVec4(color_for_head.x, color_for_head.y, color_for_head.z, 1.00f);
            colours[ImGuiCol_TitleBg] = ImVec4(color_for_area.x, color_for_area.y, color_for_area.z, 1.00f);
            colours[ImGuiCol_TitleBgCollapsed] = ImVec4(color_for_area.x, color_for_area.y, color_for_area.z, 0.75f);
            colours[ImGuiCol_TitleBgActive] = ImVec4(color_for_head.x, color_for_head.y, color_for_head.z, 1.00f);
            colours[ImGuiCol_MenuBarBg] = ImVec4(color_for_area.x, color_for_area.y, color_for_area.z, 0.47f);
            colours[ImGuiCol_ScrollbarBg] = ImVec4(color_for_area.x, color_for_area.y, color_for_area.z, 1.00f);
            colours[ImGuiCol_ScrollbarGrab] = ImVec4(color_for_head.x, color_for_head.y, color_for_head.z, 0.21f);
            colours[ImGuiCol_ScrollbarGrabHovered] = ImVec4(color_for_head.x, color_for_head.y, color_for_head.z, 0.78f);
            colours[ImGuiCol_ScrollbarGrabActive] = ImVec4(color_for_head.x, color_for_head.y, color_for_head.z, 1.00f);
            colours[ImGuiCol_CheckMark] = ImVec4(color_for_head.x, color_for_head.y, color_for_head.z, 0.80f);
            colours[ImGuiCol_SliderGrab] = ImVec4(color_for_head.x, color_for_head.y, color_for_head.z, 0.50f);
            colours[ImGuiCol_SliderGrabActive] = ImVec4(color_for_head.x, color_for_head.y, color_for_head.z, 1.00f);
            colours[ImGuiCol_Button] = ImVec4(color_for_head.x, color_for_head.y, color_for_head.z, 0.50f);
            colours[ImGuiCol_ButtonHovered] = ImVec4(color_for_head.x, color_for_head.y, color_for_head.z, 0.86f);
            colours[ImGuiCol_ButtonActive] = ImVec4(color_for_head.x, color_for_head.y, color_for_head.z, 1.00f);
            colours[ImGuiCol_Header] = ImVec4(color_for_head.x, color_for_head.y, color_for_head.z, 0.76f);
            colours[ImGuiCol_HeaderHovered] = ImVec4(color_for_head.x, color_for_head.y, color_for_head.z, 0.86f);
            colours[ImGuiCol_HeaderActive] = ImVec4(color_for_head.x, color_for_head.y, color_for_head.z, 1.00f);
            colours[ImGuiCol_ResizeGrip] = ImVec4(color_for_head.x, color_for_head.y, color_for_head.z, 0.15f);
            colours[ImGuiCol_ResizeGripHovered] = ImVec4(color_for_head.x, color_for_head.y, color_for_head.z, 0.78f);
            colours[ImGuiCol_ResizeGripActive] = ImVec4(color_for_head.x, color_for_head.y, color_for_head.z, 1.00f);
            colours[ImGuiCol_PlotLines] = ImVec4(color_for_text.x, color_for_text.y, color_for_text.z, 0.63f);
            colours[ImGuiCol_PlotLinesHovered] = ImVec4(color_for_head.x, color_for_head.y, color_for_head.z, 1.00f);
            colours[ImGuiCol_PlotHistogram] = ImVec4(color_for_text.x, color_for_text.y, color_for_text.z, 0.63f);
            colours[ImGuiCol_PlotHistogramHovered] = ImVec4(color_for_head.x, color_for_head.y, color_for_head.z, 1.00f);
            colours[ImGuiCol_TextSelectedBg] = ImVec4(color_for_head.x, color_for_head.y, color_for_head.z, 0.43f);
            colours[ImGuiCol_PopupBg] = ImVec4(color_for_pops.x, color_for_pops.y, color_for_pops.z, 0.92f);
            colours[ImGuiCol_ModalWindowDarkening] = ImVec4(color_for_area.x, color_for_area.y, color_for_area.z, 0.73f);
            break;
		case Classic:
			ImGui::StyleColorsClassic();
			break;
		case ClassicDark:
			ImGui::StyleColorsDark();
			break;
		case ClassicLight:
			ImGui::StyleColorsLight();
			break;
		case Cinder:
			colours[ImGuiCol_Text] = ImVec4(0.86f, 0.93f, 0.89f, 0.78f);
			colours[ImGuiCol_TextDisabled] = ImVec4(0.86f, 0.93f, 0.89f, 0.28f);
			colours[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.14f, 0.17f, 1.00f);
			colours[ImGuiCol_ChildWindowBg] = ImVec4(0.20f, 0.22f, 0.27f, 0.58f);
			colours[ImGuiCol_Border] = ImVec4(0.31f, 0.31f, 1.00f, 0.00f);
			colours[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
			colours[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
			colours[ImGuiCol_FrameBgHovered] = ImVec4(0.92f, 0.18f, 0.29f, 0.78f);
			colours[ImGuiCol_FrameBgActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
			colours[ImGuiCol_TitleBg] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
			colours[ImGuiCol_TitleBgCollapsed] = ImVec4(0.20f, 0.22f, 0.27f, 0.75f);
			colours[ImGuiCol_TitleBgActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
			colours[ImGuiCol_MenuBarBg] = ImVec4(0.20f, 0.22f, 0.27f, 0.47f);
			colours[ImGuiCol_ScrollbarBg] = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
			colours[ImGuiCol_ScrollbarGrab] = ImVec4(0.09f, 0.15f, 0.16f, 1.00f);
			colours[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.92f, 0.18f, 0.29f, 0.78f);
			colours[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
			colours[ImGuiCol_CheckMark] = ImVec4(0.71f, 0.22f, 0.27f, 1.00f);
			colours[ImGuiCol_SliderGrab] = ImVec4(0.47f, 0.77f, 0.83f, 0.14f);
			colours[ImGuiCol_SliderGrabActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
			colours[ImGuiCol_Button] = ImVec4(0.47f, 0.77f, 0.83f, 0.14f);
			colours[ImGuiCol_ButtonHovered] = ImVec4(0.92f, 0.18f, 0.29f, 0.86f);
			colours[ImGuiCol_ButtonActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
			colours[ImGuiCol_Header] = ImVec4(0.92f, 0.18f, 0.29f, 0.76f);
			colours[ImGuiCol_HeaderHovered] = ImVec4(0.92f, 0.18f, 0.29f, 0.86f);
			colours[ImGuiCol_HeaderActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
			colours[ImGuiCol_ResizeGrip] = ImVec4(0.47f, 0.77f, 0.83f, 0.04f);
			colours[ImGuiCol_ResizeGripHovered] = ImVec4(0.92f, 0.18f, 0.29f, 0.78f);
			colours[ImGuiCol_ResizeGripActive] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
			colours[ImGuiCol_PlotLines] = ImVec4(0.86f, 0.93f, 0.89f, 0.63f);
			colours[ImGuiCol_PlotLinesHovered] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
			colours[ImGuiCol_PlotHistogram] = ImVec4(0.86f, 0.93f, 0.89f, 0.63f);
			colours[ImGuiCol_PlotHistogramHovered] = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
			colours[ImGuiCol_TextSelectedBg] = ImVec4(0.92f, 0.18f, 0.29f, 0.43f);
			colours[ImGuiCol_PopupBg] = ImVec4(0.20f, 0.22f, 0.27f, 0.9f);
			colours[ImGuiCol_ModalWindowDarkening] = ImVec4(0.20f, 0.22f, 0.27f, 0.73f);
			break;
		default:
			break;
		}
        
        colours[ImGuiCol_TitleBg] = colours[ImGuiCol_WindowBg];
        colours[ImGuiCol_TitleBgActive] = colours[ImGuiCol_WindowBg];
        colours[ImGuiCol_TitleBgCollapsed] = colours[ImGuiCol_WindowBg];
        colours[ImGuiCol_MenuBarBg] = colours[ImGuiCol_WindowBg];
        colours[ImGuiCol_HeaderHovered] = colours[ImGuiCol_Header];
        colours[ImGuiCol_ButtonHovered] =  colours[ImGuiCol_HeaderHovered];
        colours[ImGuiCol_TabHovered] =  colours[ImGuiCol_HeaderHovered];
        colours[ImGuiCol_ButtonActive] = colours[ImGuiCol_HeaderHovered];
        colours[ImGuiCol_HeaderActive] = colours[ImGuiCol_HeaderHovered];


        
#ifdef IMGUI_HAS_DOCK
        colours[ImGuiCol_TabActive] = colours[ImGuiCol_ChildBg];
		colours[ImGuiCol_TabUnfocused] = colours[ImGuiCol_WindowBg];
#endif
	}
}
